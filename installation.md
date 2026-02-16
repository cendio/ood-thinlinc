# Installation overview
1. Configure redirection for OOD
2. Patch OOD proxy to forward HTTPS
3. Install the application on OOD
4. Install and configure the ThinLinc server on the compute node
5. Configure ThinLinc to start sessions under SLURM's control
6. Set up PAM module pam_tlpasswd for automatic login

Chapter 1-3 are for the Open OnDemand **login node** and chapter 4-6 are for
the **compute nodes**.

# 1. Configure redirection for OOD
You need to set the redirection sub-uri and the additional SSL settings for the
OOD reverse proxy in `/etc/ood/config/ood_portal.yml`:
```
ssl:
  - 'SSLCertificateKeyFile "/path/to/certs/localhost.key"'
  - 'SSLCertificateFile "/path/to/certs/localhost.crt"'
  - 'SSLProxyEngine On'
  - 'SSLProxyCheckPeerCN Off'
  - 'SSLProxyCheckPeerName off'

node_uri: '/node'
rnode_uri: '/rnode'
```

This snippet exists in a discussion Open OnDemands GitHub:

https://github.com/OSC/ondemand/issues/3179

Should you want to enable Native client support, this custom virtual host
directive will be needed to set the correct MIME-type for the ThinLinc
profile. The native client will only work if users are able to establish a
direct SSH connection to the compute node/ThinLinc server.
```
custom_vhost_directives:
  - '<LocationMatch ".*\.tlclient$">'
  - '  Header set Content-Type "application/thinlinc.client"'
  - '  Header set Content-Disposition "attachment"'
  - '</LocationMatch>'
```

When you have added these configurations to your `ood_portal.yml` config,
generate the new Apache config as such:
```
sudo /opt/ood/ood-portal-generator/sbin/update_ood_portal
```

To apply the new configurations made with `update_ood_portal`, you need to
restart the web server running the OOD instance.

# 2. Patch OOD proxy to forward HTTPS
Since the reverse proxy within Open OnDemand does not proxy forward HTTPS by
default, and downgrades the connection to HTTP, we need to apply a patch to do
this as HTTPS is a hard requirement for ThinLinc Web Access.

Patch `/opt/ood/mod_ood_proxy/lib/ood/proxy.lua` with the following command:
```
sudo patch /opt/ood/mod_ood_proxy/lib/ood/proxy.lua ./prequisites/proxy.lua.patch
```

This snippet exists in a discussion Open OnDemands GitHub:

https://github.com/OSC/ondemand/issues/3179

Without this patch, you will get stuck in a redirection loop when attempting to
connect to the ThinLinc Web Access, as the ThinLinc server will try to upgrade
your connection to HTTPS while Apache downgrades your connection to HTTP in a
loop.

# 3. Install the application on OOD
1. Clone this repository into your applications folder for OOD.

2. Configure the `form.yml`
   - You need to *at least* configure the clusters available to start the job on. These
     clusters names are the ones specified in `/etc/ood/config/clusters.d/<clustername>.yml`.

3. Configure the `submit.yml.erb` 
   - This may or may not need configurations for resources such as GPU (sharing or no
     sharing) or other devices/configuration changes made in `form.yml`.

4. Configure the `view.html.erb`
   - To enable the native client functionality, set the `enabled_client`
     variable to either `native` for native client-only support, or `both` to
     enable the native client and the web client.
   - If you configured the `rnode_uri` to something other than `/rnode`, change
     the `webaccess_url` variable to your value.

# 4. Install and configure the ThinLinc server on the compute node
Navigate to `https://www.cendio.com/thinlinc/download/` and install the latest
ThinLinc server on the compute node, then follow the setup guide on
`https://www.cendio.com/thinlinc/how-to-get-started/`.

Then, change ThinLinc Web Access' port to 4443 and restart the service:
 ```
sudo tl-config /webaccess/listen_port=4443
sudo systemctl restart tlwebaccess
```

You may need to configure SELinux and the firewall, depending on your setup.

# 5. Configure ThinLinc to start sessions under SLURM's control
Navigate to `/opt/thinlinc/etc/xsession` and overrwrite the file with the following:
```
#!/bin/bash
# -*- mode: shell-script; coding: utf-8 -*-
#
# Copyright 2002-2014 Cendio AB.
# For more information, see http://www.cendio.com

# Identify where we are running (like "lab-210.lkpg.cendio.se")
CURRENT_HOST=$(hostname)

# Look for the configuration file specific to THIS node
#    expected: ~/.thinlinc/session_config.lab-210.lkpg.cendio.se
CONFIG_FILE="$HOME/.thinlinc/.ood-secrets/session_config.${CURRENT_HOST}"

if [ -f "$CONFIG_FILE" ]; then
    source "$CONFIG_FILE"
else
    echo "No OOD session conf found" >&2
    exit 1
fi

# Check if there is an active ThinLinc job
if ! scontrol show jobid -dd "${OOD_JOB_ID}" | grep -q "thinlinc"; then
    echo "No active ThinLinc jobs"
    exit 1
fi

# Set language on Debian based systems
if [ -r /etc/default/locale ]; then
    source /etc/default/locale
    export LANG LANGUAGE LC_NUMERIC LC_TIME LC_MONETARY LC_PAPER 
    export LC_IDENTIFICATION LC_NAME LC_ADDRESS LC_TELEPHONE LC_MEASUREMENT
fi

# Log system/distribution information
source ${TLPREFIX}/libexec/log_sysinfo.sh

# Source the startup scripts with an srun wrapper
srun --jobid=${OOD_JOB_ID} /bin/bash -c "source '${TLPREFIX}/etc/xstartup.default'"
```

# 6. Set up PAM module pam_tlpasswd for automatic login
This chapter contains two steps. Installing the PAM module, then installing a
Slurm Epilog script to clean up the temporary password files created by the Open
OnDemand job. Both of these steps are done on the **Compute nodes**.

## Install the PAM module pam_tlpasswd
1. [Download](https://github.com/cendio/ood-thinlinc/releases/download/v1.0/pam_tlpasswd.so) or [build](/prequisites/pam_tlpasswd) the PAM module `pam_tlpasswd`.

2. Install the PAM module `pam_tlpasswd`, you may need to reconfigure the target path to
   your PAM modules directory. *The target path may differ depending on your distro.*
```
sudo install pam_tlpasswd.so /lib64/security/pam_tlpasswd.so
```

3. Configure `/etc/pam.d/sshd` to add this in the top of the file:
```
auth	   [success=done ignore=ignore default=die] pam_tlpasswd.so
```

## Install the Slurm Epilog clean up script
1. [Download](https://github.com/cendio/ood-thinlinc/releases/download/v1.0/ood_thinlinc_cleanup.sh)
   or scp the [clean up script](/prequisites/ood_thinlinc_cleanup.sh).

2. Install the clean up script. *The target path may be any other place, just make sure it matches the path in step 3*
```
sudo cp ood_thinlinc_cleanup.sh /etc/slurm/ood_thinlinc_cleanup.sh
```

3. Edit `/etc/slurm/slurm.conf` to add the following line:
```
Epilog=/etc/slurm/ood_thinlinc_cleanup.sh
```

Debugging The Epilog clean up script:
```
sudo journalctl -t tlCleanupEpilog
```
