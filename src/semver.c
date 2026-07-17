#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "semver.h"

// 跳过前导空白
static const char *skip_leading_spaces(const char *s)
{
  while (*s && isspace((unsigned char)*s))
    s++;
  return s;
}

// 解析一个非负整数，成功返回下一个要处理的位置，失败返回 NULL
static const char *parse_nonneg_int(const char *s, int *out)
{
  if (!isdigit((unsigned char)*s))
    return NULL;
  *out = (int)strtol(s, (char **)&s, 10);
  return s;
}

/**
 * @brief 解析并验证预发布标识符（'-' 之后的部分）
 * @param s          指向 '-' 之后第一个字符的指针
 * @param prerelease 输出缓冲区
 * @param max_len    缓冲区大小
 * @return 0 成功，-1 格式错误
 */
static int parse_prerelease(const char *s, char *prerelease, size_t max_len)
{
  const char *p = s;
  // 扫描到 '+' 或字符串结尾，同时检查字符合法性
  while (*p && *p != '+')
  {
    // 仅允许字母、数字、连字符和点
    if (!isalnum((unsigned char)*p) && *p != '-' && *p != '.')
    {
      return -1;
    }
    p++;
  }

  size_t len = p - s;
  if (len == 0)
    return -1; // 空的预发布，如 "1.2.3-"

  // 按 '.' 切分标识符，逐段验证
  const char *start = s;
  while (start < p)
  {
    const char *end = start;
    while (end < p && *end != '.')
      end++;

    size_t id_len = end - start;
    if (id_len == 0)
      return -1; // 连续点或开头点，如 "1.2.3-."、"1.2.3-alpha..1"

    // 检查是否为纯数字标识符
    int all_digits = 1;
    for (const char *ch = start; ch < end; ch++)
    {
      if (!isdigit((unsigned char)*ch))
      {
        all_digits = 0;
        break;
      }
    }
    // 数值标识符不能有前导零（"0" 除外）
    if (all_digits && id_len > 1 && *start == '0')
    {
      return -1;
    }

    start = end;
    if (start < p)
      start++; // 跳过 '.'
  }

  // 复制到输出缓冲区
  if (len >= max_len)
    len = max_len - 1;
  memcpy(prerelease, s, len);
  prerelease[len] = '\0';
  return 0;
}

int semver_parse(const char *ver_str, semver_t *sem)
{
  if (!ver_str || !sem)
    return -1;

  // 初始化输出
  memset(sem, 0, sizeof(*sem));

  ver_str = skip_leading_spaces(ver_str);
  if (*ver_str == '\0')
    return -1;

  // 可选前缀 'v' 或 'V'
  if (*ver_str == 'v' || *ver_str == 'V')
    ver_str++;

  // 解析 major
  const char *p = parse_nonneg_int(ver_str, &sem->major);
  if (!p || *p != '.')
    return -1;

  // 解析 minor
  p = parse_nonneg_int(p + 1, &sem->minor);
  if (!p || *p != '.')
    return -1;

  // 解析 patch
  p = parse_nonneg_int(p + 1, &sem->patch);
  if (!p)
    return -1; // patch 必须是数字，且至少一位

  // 处理预发布标签
  if (*p == '-')
  {
    if (parse_prerelease(p + 1, sem->prerelease, SEMVER_PRERELEASE_MAX_LEN) != 0)
      return -1;
  }
  else if (*p == '+' || *p == '\0' || isspace((unsigned char)*p))
  {
    sem->prerelease[0] = '\0'; // 无预发布
  }
  else
  {
    return -1; // 无效字符
  }

  return 0;
}

// 比较两个预发布字段
static int compare_prerelease(const char *a, const char *b)
{
  // 复制字符串以便分割
  char a_buf[SEMVER_PRERELEASE_MAX_LEN];
  char b_buf[SEMVER_PRERELEASE_MAX_LEN];
  strncpy(a_buf, a, sizeof(a_buf) - 1);
  strncpy(b_buf, b, sizeof(b_buf) - 1);
  a_buf[sizeof(a_buf) - 1] = '\0';
  b_buf[sizeof(b_buf) - 1] = '\0';

  char *save_a, *save_b;
  char *tok_a = strtok_r(a_buf, ".", &save_a);
  char *tok_b = strtok_r(b_buf, ".", &save_b);

  while (tok_a || tok_b)
  {
    if (!tok_a && tok_b)
      return -1; // a 较短 => a < b
    if (tok_a && !tok_b)
      return 1; // b 较短 => a > b

    // 判断是否纯数字
    int is_a_num = 1, is_b_num = 1;
    for (char *ch = tok_a; *ch; ch++)
    {
      if (!isdigit((unsigned char)*ch))
      {
        is_a_num = 0;
        break;
      }
    }
    for (char *ch = tok_b; *ch; ch++)
    {
      if (!isdigit((unsigned char)*ch))
      {
        is_b_num = 0;
        break;
      }
    }

    if (is_a_num && is_b_num)
    {
      int val_a = atoi(tok_a);
      int val_b = atoi(tok_b);
      if (val_a != val_b)
        return (val_a > val_b) ? 1 : -1;
    }
    else if (!is_a_num && !is_b_num)
    {
      int cmp = strcmp(tok_a, tok_b);
      if (cmp != 0)
        return (cmp > 0) ? 1 : -1;
    }
    else
    {
      // 数字标识符优先级低于非数字
      return is_a_num ? -1 : 1;
    }

    tok_a = strtok_r(NULL, ".", &save_a);
    tok_b = strtok_r(NULL, ".", &save_b);
  }
  return 0;
}

int semver_compare(const semver_t *a, const semver_t *b)
{
  if (!a || !b)
    return 0;

  if (a->major != b->major)
    return (a->major > b->major) ? 1 : -1;
  if (a->minor != b->minor)
    return (a->minor > b->minor) ? 1 : -1;
  if (a->patch != b->patch)
    return (a->patch > b->patch) ? 1 : -1;

  int a_has_pre = (a->prerelease[0] != '\0');
  int b_has_pre = (b->prerelease[0] != '\0');

  if (!a_has_pre && !b_has_pre)
    return 0;
  if (!a_has_pre && b_has_pre)
    return 1;
  if (a_has_pre && !b_has_pre)
    return -1;

  return compare_prerelease(a->prerelease, b->prerelease);
}

int semver_compare_str(const char *ver_a, const char *ver_b)
{
  semver_t sem_a, sem_b;

  if (semver_parse(ver_a, &sem_a) != 0)
    return -1;
  if (semver_parse(ver_b, &sem_b) != 0)
    return -1;
  return semver_compare(&sem_a, &sem_b);
}