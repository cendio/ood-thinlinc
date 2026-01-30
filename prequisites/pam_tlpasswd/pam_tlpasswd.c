#include <security/_pam_types.h>
#include <security/pam_ext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <security/pam_appl.h>
#include <security/pam_modules.h>
#include "pam_modutil.h"
#include <errno.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <crypt.h>
#include <sys/types.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_TOKEN_PART_LEN 511

void send_user_msg(pam_handle_t *pamh, const char *msg_str) {
    struct pam_conv *conv = NULL;
    struct pam_message msg;
    const struct pam_message *msg_ptr = &msg;
    struct pam_response *resp = NULL;
    int pam_err;

    pam_err = pam_get_item(pamh, PAM_CONV, (const void **)&conv);
    if (pam_err != PAM_SUCCESS || conv == NULL || conv->conv == NULL) {
        return;
    }

    msg.msg_style = PAM_ERROR_MSG;
    msg.msg = msg_str;

    conv->conv(1, &msg_ptr, &resp, conv->appdata_ptr);
    if (resp) {
        if (resp->resp) {
            free(resp->resp);
        }
        free(resp);
    }
}

char* read_job_password_hash_file(pam_handle_t *pamh, const char *userHome, const char *job_id) {
    const char *filename_prefix = ".thinlinc/.ood-secrets/job-";
    const char *log_suffix = "read_job_password_hash_file:";
    size_t buffer_size = strlen(userHome) + 1 + strlen(filename_prefix) + strlen(job_id) + 1;
    char *fileSource = malloc(buffer_size);

    if (fileSource == NULL) {
	pam_syslog(pamh, LOG_ERR, "%s malloc fail!", log_suffix);
        return NULL;
    }

    snprintf(fileSource, buffer_size, "%s/%s%s", userHome, filename_prefix, job_id);

    struct stat st;
    if (stat(fileSource, &st) != 0) {
        pam_syslog(pamh, LOG_ERR, "%s stat() failed for %s: %s", log_suffix, fileSource, strerror(errno));
        send_user_msg(pamh, "Could not access temporary password file.");
        free(fileSource);
        fileSource = NULL;
        return NULL;
    }

    if ((st.st_mode & 0777) != 0600) {
        pam_syslog(pamh, LOG_ERR, "%s Insecure permissions on file %s. Should be 600.", log_suffix, fileSource);
        send_user_msg(pamh, "Temporary password file has insecure permissions.");
        free(fileSource);
        fileSource = NULL;
        return NULL;
    }

    FILE *file_handle = fopen(fileSource, "r");
    free(fileSource);
    fileSource = NULL;

    if (file_handle == NULL) {
        pam_syslog(pamh, LOG_ERR, "%s Could not open file: %s\n", log_suffix, strerror(errno));
	send_user_msg(pamh, "Could not find a valid temporary password file.");
        return NULL;
    }

    char line_buffer[512];
    if (fgets(line_buffer, sizeof(line_buffer), file_handle) != NULL) {
        fclose(file_handle);
        line_buffer[strcspn(line_buffer, "\n")] = '\0';

        if (line_buffer[0] == '\0') {
            pam_syslog(pamh, LOG_ERR, "%s File was empty after stripping newline.", log_suffix);
            return NULL;
        }

        char *return_string = strdup(line_buffer);
        if (return_string == NULL) {
            pam_syslog(pamh, LOG_ERR, "%s strdup failed to allocate memory.", log_suffix);
            return NULL;
        }
        return return_string;
    } else {
	send_user_msg(pamh, "Could not read the temporary password from file.");
        pam_syslog(pamh, LOG_ERR, "%s File was empty or could not be read.", log_suffix);
        fclose(file_handle);
        return NULL;
    }
}

static int verify_password(pam_handle_t *pamh,
			   char *cleartext_secret,
			   char *expected_hash) {
    const char *log_suffix = "verify_password:";

    if (!cleartext_secret || !expected_hash) {
        pam_syslog(pamh, LOG_ERR, "%s Received NULL input.", log_suffix);
        return PAM_AUTH_ERR;
    }

    char *hashed_secret_from_user = crypt(cleartext_secret, expected_hash);
    if (hashed_secret_from_user == NULL) {
        pam_syslog(pamh, LOG_ERR, "%s crypt() function failed.", log_suffix);
        return PAM_AUTH_ERR;
    }

    int result = PAM_AUTH_ERR;
    if (strcmp(hashed_secret_from_user, expected_hash) == 0) {
        result = PAM_SUCCESS;
    }

    if (result != PAM_SUCCESS) {
        pam_syslog(pamh, LOG_ERR, "%s Password verification failed.", log_suffix);
	send_user_msg(pamh, "Temporary token verification failed.");
    }

    return result;
}

const char *verify_and_set_token(pam_handle_t *pamh) {
    int pam_err;
    const char *log_suffix = "verify_and_set_token:";
    const char *token = NULL; // This will be "job id:password hash"

    pam_err = pam_get_authtok(pamh, PAM_AUTHTOK, &token, NULL);
    if (pam_err != PAM_SUCCESS) {
        pam_syslog(pamh, LOG_ERR, "%s Error getting auth token!", log_suffix);
        return NULL;
    }

    if (token == NULL) {
        pam_syslog(pamh, LOG_ERR, "%s Password (PAM_AUTHTOK) is NULL.", log_suffix);
        return NULL;
    }

    const char *colon_ptr = strchr(token, ':');
    if (colon_ptr == NULL) {
        pam_syslog(pamh, LOG_ERR, "%s No colon in token.", log_suffix);
	send_user_msg(pamh, "Incorrect token format.");
        return NULL;
    }

    return token;
}

static int check_pam_service(pam_handle_t *pamh) {
    const char *log_suffix = "check_pam_service:";
    const char *allowed = "thinlinc";
    char *service = NULL;
    int rc;

    rc = pam_get_item(pamh, PAM_SERVICE, (void*)&service);
    if (rc != PAM_SUCCESS) {
	pam_syslog(pamh, LOG_ERR, "%s Failed to obtain PAM_SERVICE name", log_suffix);
	return rc;
    }

    if (!strcmp(service, allowed)) {
	pam_syslog(pamh, LOG_INFO,
		   "%s Authenticating with pam_tlpasswd since this is ThinLinc", log_suffix);
	return PAM_SUCCESS;
    }

    pam_syslog(pamh, LOG_INFO,
	       "%s Not authenticating with pam_tlpasswd since this is not ThinLinc", log_suffix);
    return PAM_IGNORE;
}

PAM_EXTERN int pam_sm_authenticate(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    int retval = PAM_IGNORE;
    retval = check_pam_service(pamh);
    if (retval != PAM_SUCCESS) {
	return retval;
    }

    int pam_err;
    const char *username = NULL;
    const char *log_suffix = "pam_sm_authenticate:";

    pam_err = pam_get_user(pamh, &username, NULL);
    if (pam_err != PAM_SUCCESS || username == NULL) {
        pam_syslog(pamh, LOG_ERR, "%s Could not get username!", log_suffix);
        return PAM_USER_UNKNOWN;
    }

    struct passwd *pw = NULL;
    pw = pam_modutil_getpwnam(pamh, username);
    if (!pw) {
        pam_syslog(pamh, LOG_ERR, "%s Didnt find passwd for user %s!", username, log_suffix);
        return PAM_USER_UNKNOWN;
    }

    const char *token = verify_and_set_token(pamh);
    if (token == NULL) {
	return PAM_AUTH_ERR;
    }

    char job_id[MAX_TOKEN_PART_LEN + 1];
    char cleartext_secret_from_user[MAX_TOKEN_PART_LEN + 1];
    job_id[0] = '\0';
    cleartext_secret_from_user[0] = '\0';
    sscanf(token, "%511[^:]:%511s", job_id, cleartext_secret_from_user);

    char *expected_hash_from_file = read_job_password_hash_file(pamh, pw->pw_dir, job_id);
    if (!expected_hash_from_file) {
        pam_syslog(pamh, LOG_ERR, "%s Couldnt read password hash from file, or file was empty.", log_suffix);
        return PAM_AUTH_ERR;
    }

    int result = verify_password(pamh, cleartext_secret_from_user, expected_hash_from_file);

    free(expected_hash_from_file);
    expected_hash_from_file = NULL;

    return result;
}

PAM_EXTERN int pam_sm_setcred(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}

PAM_EXTERN int pam_sm_acct_mgmt(pam_handle_t *pamh, int flags, int argc, const char **argv) {
    return PAM_SUCCESS;
}
