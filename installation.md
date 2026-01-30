# Installation overview
1. Configure SSL redirection for OOD
2. Install the application on OOD
3. Install and configure the ThinLinc server on the compute node
4. Set up PAM module pam_tlpasswd for automatic login

Chapter 1-2 are for the Open OnDemand **login node** and chapter 3-4 are for
the **compute nodes**.

# 1. Configure SSL redirection for OOD
You need to set the SSL redirection sub-uri and the additional SSL settings for
the OOD reverse proxy in `/etc/ood/config/ood_portal.yml`:
```
secure_rnode_uri: '/secure-rnode'
ssl_proxy:
  - 'SSLProxyCheckPeerCN Off'
  - 'SSLProxyCheckPeerName Off'
```

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

# 2. Install the application on OOD
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
   - If you configured the `secure_rnode_uri` to something other than
     `/secure-rnode`, change the `webaccess_url` variable to your value.

# 3. Install and configure the ThinLinc server on the compute node
[Download](https://www.cendio.com/thinlinc/download/) and
[install](https://www.cendio.com/thinlinc/docs/install/) the latest ThinLinc
server on the **compute node**.

# 4. Set up PAM module pam_tlpasswd for automatic login
This chapter contains two steps. Installing the PAM module, then installing a
Slurm Epilog script to clean up the temporary password files created by the Open
OnDemand job. Both of these steps are done on the **Compute nodes**.

## Install the PAM module pam_tlpasswd
1. Install the PAM module `pam_tlpasswd`, you need to reconfigure the path to
   your PAM modules directory.
```
sudo install ./prequisites/pam_tlpasswd/pam_tlpasswd.so /lib64/security/pam_tlpasswd.so
```

2. Configure `/etc/pam.d/sshd` to add this in the top of the file:
```
auth	   [success=done ignore=ignore default=die] pam_tlpasswd.so
```

## Install the Slurm Epilog clean up script
1. Install the clean up script
```
sudo cp ./prequisites/ood_thinlinc_cleanup.sh /etc/slurm/ood_thinlinc_cleanup.sh
```

2. Edit `/etc/slurm/slurm.conf` to add the following line:
```
Epilog=/etc/slurm/ood_thinlinc_cleanup.sh
```

Debugging The Epilog clean up script:
```
sudo journalctl -t tlCleanupEpilog
```
