#ifndef PAM_MODUTIL_H
# define PAM_MODUTIL_H

#include <security/pam_modules.h>
#include <security/_pam_macros.h>
#include <security/_pam_types.h>
# ifdef HAVE_SECURITY_PAM_MODUTIL_H
#  include <security/pam_modutil.h>
# else

#  ifdef HAVE_SECURITY_PAM_MODULES_H
#   include <security/pam_modules.h>
#  endif

#  include <pwd.h>

struct passwd *pam_modutil_getpwnam (pam_handle_t * pamh, const char *user);
extern void send_user_msg(pam_handle_t *pamh, const char *msg);

# endif
#endif
