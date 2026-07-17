#ifndef __SEMVER_H__
#define __SEMVER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 支持的预发布标识最大长度
 */
#define SEMVER_PRERELEASE_MAX_LEN 64

/**
 * @brief SemVer 版本结构体
 */
typedef struct {
    int major;
    int minor;
    int patch;
    char prerelease[SEMVER_PRERELEASE_MAX_LEN];  // 例如 "beta.1"，空串表示正式版
} semver_t;

/**
 * @brief 解析版本字符串，支持以下格式：
 *        - 1.2.3
 *        - v1.2.3
 *        - v1.2.3-beta.1
 *        - 1.2.3-alpha+001
 *        构建元数据（'+' 之后）会被忽略
 * @param ver_str 输入字符串
 * @param sem     输出结构体
 * @return 0 成功，-1 格式错误
 */
int semver_parse(const char *ver_str, semver_t *sem);

/**
 * @brief 比较两个 SemVer 版本（遵循 SemVer 2.0 优先级规则）
 * @param a 第一个版本
 * @param b 第二个版本
 * @return  1 若 a > b
 *          0 若 a == b
 *         -1 若 a < b
 */
int semver_compare(const semver_t *a, const semver_t *b);

/**
 * @brief 比较两个版本字符串（自动解析后比较）
 * @param ver_a 版本字符串 A
 * @param ver_b 版本字符串 B
 * @return 同 semver_compare
 */
int semver_compare_str(const char *ver_a, const char *ver_b);

#ifdef __cplusplus
}
#endif

#endif /* __SEMVER_H__ */
