# Authentication
When a user's ThinLinc session starts through Open OnDemand, ThinLinc needs to
authenticate the user before granting access to the desktop. There are two
supported options:

1. **No automatic login**: the user is presented with the ThinLinc Web Access
   login prompt and authenticates manually.
2. **Automatic login** using the custom PAM module `pam_tlpasswd`: the user is
   logged in automatically, without being prompted.

Choose the option that best fits your environment. Which option is used is
controlled by the `auto_login` variable in `view.html.erb`, which is `false` by
default (Option 1).

# Option 1: No automatic login
This is the default behaviour and requires no additional installation. With
`auto_login = false` in `view.html.erb`, the "Connect using the browser" button
simply opens ThinLinc Web Access, and the user authenticates at the Web Access
login prompt with their own system credentials.

If this is acceptable for your users, there is nothing more to configure and you
can skip Option 2 entirely.

# Option 2: Automatic login with the PAM module pam_tlpasswd
With this option the user is logged in to their ThinLinc session automatically,
without being shown the ThinLinc Web Access login prompt.

This consists of two steps: enabling automatic login in the application, then
installing the PAM module on the **compute nodes**. The temporary password files
this creates are cleaned up by the Slurm Epilog script from
[installation step 4](installation.md), which you have already installed.

## Enable automatic login in the application
In `view.html.erb`, set the `auto_login` constant to `true`:
```
auto_login = true
```
With this enabled, the "Connect using the browser" button posts the temporary
password to ThinLinc Web Access, where `pam_tlpasswd` validates it and logs the
user in automatically. **Only enable this once the PAM module below is installed**,
otherwise the login POST has nothing to validate against and will fail.

## Install the PAM module pam_tlpasswd
1. [Download](https://github.com/cendio/ood-thinlinc/releases/download/v1.0/pam_tlpasswd.so)
   or [build](/prequisites/pam_tlpasswd) the PAM module `pam_tlpasswd`.

2. Install the PAM module `pam_tlpasswd`, you may need to reconfigure the target path to
   your PAM modules directory. *The target path may differ depending on your distro.*
```
sudo install pam_tlpasswd.so /lib64/security/pam_tlpasswd.so
```

3. Configure `/etc/pam.d/sshd` to add this in the top of the file:
```
auth	   [success=done ignore=ignore default=die] pam_tlpasswd.so
```
