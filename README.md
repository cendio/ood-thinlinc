# ThinLinc for Open OnDemand (Beta Version)
Welcome to the beta release of ThinLinc for Open OnDemand (OOD). 

This repository contains everything you need to get started with using ThinLinc through Open OndDemand in a Slurm cluster. This repository is targeted at system administrators as it requires elevated privileges to install ThinLinc and the OOD ThinLinc application. If you are a user of an OOD system, please contact the administrator and ask for installation of this app.

## Requirements
- Open OnDemand version 4.1
  - It is possible to run this application on earlier Open OnDemand versions, though it will require manual patching of the Open OnDemand `mod_ood_proxy` module. If this is preferred, see the [pre-4.1-ood](https://github.com/cendio/ood-thinlinc/tree/pre-4.1-ood) branch.
- ThinLinc version 4.20
- Slurm as the batch system
  - We have tested with Slurm version `22.05`, but it should work with older versions as well.
- X86 compute nodes with Linux

## What is ThinLinc for Open OnDemand
This beta version provides functionality that is similar to what the build-in "interactive desktop" function of OOD provides, but implemented using ThinLinc. Using ThinLinc also enables connecting to the desktop from the ThinLinc native clients rather than just through the web browser.

In a future version, we plan to expand the functionality to also run ThinLinc on servers outside the batch system, to enable truly persistent desktops that users can log-in for days and weeks. This will make it possible to build an HPC Desktop through OOD.

## Why a beta release / What are we looking for
The primary goal of publishing  a beta version is to get feedback from the community. Please submit a feedback form if you have installed the beta version and let us know how it went. We are looking for feedback, even if everything went well and you didn't encounter any issues during setup or usage.

[Link to the feedback form](https://docs.google.com/forms/d/e/1FAIpQLSc29FNqfOW0E0d84nUfwJY_Qh0nWHOBSKOVxskCmp6Sa8Sg1w/viewform?usp=header)

## Getting Started
Please read the [Installation guide](installation.md).

## Note on the ThinLinc End User License Agreement
The ThinLinc End User License Agreement restricts ThinLinc the free usage of ThinLinc to 10 concurrent users. If your organization doesn't already have a ThinLinc license and you expect more than 10 users, please [contact](mailto:contact@cendio.com) us. We are happy to provide evaluation licenses for testing out this beta version.

## Submitting Feedback / Bugs / Issues
Please use the issue tracker in this repository to submit feedback and open issues.

If you already have a ThinLinc license and an active support agreement, and you have site specific issues, you can also open a support ticket through the usual [ThinLinc support](https://www.cendio.com/support/) channel. 
