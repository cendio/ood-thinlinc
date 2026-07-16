# Installation overview
1. Configure SSL redirection for OOD
2. Install the application on OOD
3. Install and configure the ThinLinc server on the compute node
4. Configure ThinLinc to start sessions under SLURM's control
5. Configure authentication for ThinLinc sessions (optional)

Chapter 1-2 are for the Open OnDemand **login node** and chapter 3-5 are for
the **compute nodes**.

# 1. Configure SSL redirection for OOD
You need to set the SSL redirection sub-uri and the additional SSL settings for
the OOD reverse proxy in `/etc/ood/config/ood_portal.yml`:
```
secure_rnode_uri: '/secure-rnode'
ssl_proxy:
  - 'SSLProxyCheckPeerCN Off'
  - 'SSLProxyCheckPeerName Off'

custom_location_directives:
  - '<If "%{REQUEST_URI} =~ m|^/secure-rnode/([^/]+)/(\d+)/connect/\1|">'
  - '  AddOutputFilterByType SUBSTITUTE text/html application/javascript'
  - '  Substitute "s|https://([^/:]+):(\d+)/|/secure-rnode/$1/$2/|i"'
  - '</If>'
```

**NOTE**: Read about why the `custom_location_directives` is set in the [README](README.MD).

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

# 4. Configure ThinLinc to start sessions under SLURM's control
Navigate to `/opt/thinlinc/etc/xsession` and overrwrite the file with the
contents in [the provided xsession file](/prequisites/xsession). *Also available
as a [direct download](https://github.com/cendio/ood-thinlinc/releases/download/v1.0/xsession).*

## Install the Slurm Epilog clean up script
Each session writes a small per-node config file
(`~/.thinlinc/.ood-secrets/session_config.<host>`, containing the job id and node)
that the xsession reads to bind the desktop to the correct SLURM job. A Slurm
Epilog script removes these files when the job ends. This is **required regardless
of which authentication option** you choose in the next chapter — the automatic
login option (Option 2) reuses the same script to also clean up its temporary
password files.

1. [Download](https://github.com/cendio/ood-thinlinc/releases/download/v1.0/ood_thinlinc_cleanup.sh)
   or scp the [clean up script](/prequisites/ood_thinlinc_cleanup.sh).

2. Install the clean up script. *The target path may be any other place, just
   make sure it matches the path in step 3*
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

# 5. Configure authentication for ThinLinc sessions (optional)
When a user connects to their ThinLinc session, they need to authenticate. There
are two supported options: no automatic login, which presents the ThinLinc Web
Access login prompt without any setup required, or automatic login using the
custom PAM module `pam_tlpasswd`.

See the [authentication document](authentication.md) for a description of both
options and how to set them up.
