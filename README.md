example-bosh-addon
==================

What's all this, then
---------------------

This is an example BOSH release that I wrote to help me understand how BOSH addons work.

I want to document a few things:

1. What's in this repository
2. Why it's in this repository
3. How I tested
4. Other tips

I know this readme document is big.  There is no TL,DR because you have to R.  This doc is intended to be read from top to bottom, and not as a reference.  At least not the first time.

Hopefully, this will be helpful for understanding how a BOSH _package_, _job_, _release_, _deployment_ and _runtime configs_ work in a very simple example.  To really get this running in BOSH, you need everything I mentioned in the previous sentance.  I'll try to break it down piece-by-piece, but understand that we can't really test it until it's all in place.

Anything that I want you to actually run will be in a code block, like this:

```bash
$ ./magic-eight-ball --question "Is this repository going to be helpful to anyone?"
Ask again later.
```

These blocks assume that you have a BOSH environment already setup, either running on a cloud somewhere, or using [virtual box](https://bosh.io/docs/quick-start/).

number-picker
-------------

At the heart of this project is a very simple "service".  I wanted to make something with some really simple requirements:

1. Simple to understand
2. Stays running persistent (not just a one-time job)
3. Easy to see that it's running
4. No dependencies
5. Portable

Here's why these are important:

\#1 is easy to understand. \#2 is important because I want to see the process running on the BOSH VMs. \#3 lets me test job logging.  \#4 and \#5 are great for testing the addon on multiple platforms.

The simple "service" I came up with is **number-picker**.

Number Picker is a C++ program that will:

1. Chooses a random number
2. Sleep for a period of time
3. Output that number to stdout
4. goto 1

This program will never exit (satisfying requirement \#2), so it emulates a "service".

The sleep interval can be set in two ways:

1. Use the default (every two seconds)
2. Provide a new value on the `NUMBER_PICKER_INTERVAL` environment variable

Try this and make sure you understand what this program is doing:

```bash
$ cd src
$ make
$ ./number-picker  # Should output a number every two seconds
My number is 16807
My number is 282475249
...
<Ctrl-C>
$ export NUMBER_PICKER_INTERVAL=5
$ ./number-picker  # Should output a number every five seconds
My number is 16807
My number is 282475249
...
<Ctrl-C>
$ cd -
```

Notice that this only uses the `src` directory

BOSH package
------------

Up to this point, nothing has been BOSH specific.  The first part in understanding this repository is understanding the package.

A BOSH package describes some artifact(s) that BOSH needs to put into the VM.  There's no logic on how to run it, or where to run it.  Just the files to include.

This repository has one package, named `number-picker`.

The `packages/number-picker/spec` file describes the package, giving it a name and listing the files that should be included on the BOSH _compilation_ VM.  These files are expected to be inside the `src` directory.  More on the compilation VM in a sec.

The `packages/number-picker/packaging` file is a script that runs on the BOSH _compilation_ VM that compiles the C++ file into a binary.  It then copies the binary into the directory specified by the `BOSH_INSTALL_TARGET` environment variable.  This location is special, as the files inside will be available on the running VM.

On the running vm, you can find the compiled binary at `/var/vcap/packages/number-picker/number-picker`

BOSH job
--------

Now that we have the package, we need to know how to run it.  The BOSH job is a process that needs to run, either one-time (called _errands_) or persistantly.

This repository has one job, also named `number-picker`.

The `jobs/number-picker/spec` file describes the job.  It specifies the job name, the package that it requires (our `number-picker` package), some template files (more about this later), and property definitions.

The property definition is how we declare a custom property, `number-picker.interval` that we will eventually set in an environment variable to set the sleep interval like we did when we ran `number-picker` locally.  We set the default value here (i.e. twenty seconds).  Note that this default is distinct from the default set inside of `src/number-picker.cpp`.  Later on, we'll set this property to a new value.

The files in `jobs/number-picker/templates` are [ERB](http://ruby-doc.org/stdlib-2.5.1/libdoc/erb/rdoc/ERB.html) templates that resolve into bash scripts.  The `start.sh.erb` file will become `start.sh` when deployed and the one that starts the `number-picker` "service", saving the process PID into a PIDFILE.  This script also sets the `NUMBER_PICKER_INTERVAL` environment variable to the value of the `number-picker.interval` property that we defined in the job spec.

Similarly, the `stop.sh.erb` becomes `stop.sh` and is used to stop the process.

The `jobs/number-picker/monit` file is a [monit](https://mmonit.com/monit/) script.  BOSH uses monit to observe the health of the processes.  It will restart them when they go down, and if they don't start or they don't stay up, it will re-create the VM.

On the running vm, you can find the job scripts:

* `/var/vcap/jobs/number-picker/monit`
* `/var/vcap/jobs/number-picker/bin/start.sh`
* `/var/vcap/jobs/number-picker/bin/stop.sh`

BOSH release
------------

The BOSH release encapsulates our package and job into a single, versioned, bundle.  Each time you create a release, it gets a unique version number, which is very 12 factor.

The `config/final.yml` file is only used to name the release.
`config/blobs.yml` is only there to say we don't use any blobstores (`bosh create-release` complains if this file is missing).

Let's create a release by running:

```bash
$ bosh create-release
Adding package 'number-picker/0e677d659f71519dd7eadbe13bb19fdc25750eec'...
Adding job 'number-picker/4a5f85eaeaa4a7b015b3f74462c5c49f03ca4349'...
Added package 'number-picker/0e677d659f71519dd7eadbe13bb19fdc25750eec'
Added job 'number-picker/4a5f85eaeaa4a7b015b3f74462c5c49f03ca4349'

Added dev release 'example-addon-release/0+dev.1'

Name         example-addon-release  
Version      0+dev.1  
Commit Hash  5a2fa47+  

Job                                                     Digest                                    Packages  
number-picker/4a5f85eaeaa4a7b015b3f74462c5c49f03ca4349  cf1e19b33b2d1b543127b649ee9d105514564c2c  number-picker  

1 jobs

Package                                                 Digest                                    Dependencies  
number-picker/0e677d659f71519dd7eadbe13bb19fdc25750eec  842fd4f5840d48493b75c32f0631679a53507129  -  

1 packages

Succeeded
```

After this step, we need to upload it, so run:

```bash
$ bosh upload-release
Using environment '192.168.50.6' as client 'admin'

######################################################### 100.00% 28.27 KiB/s 0s
Task 211

Task 211 | 22:29:51 | Extracting release: Extracting release (00:00:00)
Task 211 | 22:29:51 | Verifying manifest: Verifying manifest (00:00:00)
Task 211 | 22:29:51 | Resolving package dependencies: Resolving package dependencies (00:00:00)
Task 211 | 22:29:51 | Creating new packages: number-picker/0e677d659f71519dd7eadbe13bb19fdc25750eec (00:00:00)
Task 211 | 22:29:51 | Creating new jobs: number-picker/4a5f85eaeaa4a7b015b3f74462c5c49f03ca4349 (00:00:00)
Task 211 | 22:29:51 | Creating new jobs: smoke-tests/03fd57cb0e2f383243492a794c8efa5de09ec732 (00:00:00)
Task 211 | 22:29:51 | Release has been created: example-addon-release/0+dev.2 (00:00:00)

Task 211 Started  Tue Sep 11 22:29:51 UTC 2018
Task 211 Finished Tue Sep 11 22:29:51 UTC 2018
Task 211 Duration 00:00:00
Task 211 done

Succeeded
```

You should be able to see the releases by running `bosh releases`

BOSH deployment
---------------

Even though this repository is about making a BOSH addon, we're going to deploy it on it's own first.  Why?  Addons require other deployments to work.  A standalone deployment makes it quicker to test changes and see `number-picker` in action.

The `manifest-deployment.yml` file contains a deployment manifest for our release.  The important thing for us in this file are the deplyment name:

```yml
name: example-addon-standalone
```

The `release` section, which references our release:

```yml
releases:
- name: example-addon-release
  version: latest
```

The `job` section in the `instance_group` section, which defines the jobs included in the instances and also sets the `number-picker.interval` property to five seconds:

```yml
jobs:
  - name: number-picker
    release: example-addon-release
    properties:
      number-picker:
      interval: 5
```

BOSH runtime config
-------------------

Now, let's actually make it into a BOSH addon.  Rather than using a deplyment manifest to put our `number-picker` on it's own, we're going to update BOSH's runtime config to tell it to include it on other deployments in the system.

The `manifest-addon.yml` file contains the new runtime-config that will add `number-picker` to new deployments.  Again, we specify the release in the `releases` section, however this time we are not allowed to use the `latest` value for version.  Keep in mind that everytime you `bosh create-release`, you'll need to update this file.

The `job` section in the `addons` section also specifies the same job configuration and provides the same value for the property.  It's worth noting that we don't *need* to provide this property value, it's just useful for showing how we can override the defaults.

The `job` section also specifies an `include` and an `exclude` section.  This is how we tell BOSH which VMs should have this addon installed.

Looking at the VM
-----------------

Frequently, when testing this, it is very useful to see what's actually happening on the VMs themselves.  To do this, you first find the instance name by running:

```bash
$ bosh vms
Using environment '192.168.50.6' as client 'admin'

Task 233. Done

Deployment 'example-addon-standalone'

Instance                                                  Process State  AZ  IPs         VM CID                                VM Type  Active  
example-addon-group/7cddf3e8-5785-43d1-b999-a082d421e984  running        z1  10.244.0.3  c46b982f-48b8-4460-580f-f3b9ace9163f  default  true  

1 vms

Succeeded
$ bosh ssh --deployment=example-addon-standalone 
...
example-addon-group/7cddf3e8-5785-43d1-b999-a082d421e984:~$
<Ctrl-D>
$
```

Troubleshooting
---------------

When things go wrong, what to check?  Here's some ideas:

* Check if the VM is running
  * `bosh vms`
* Log into the VM and look at the following:
  * `monit summary`
  * `/var/vcap/sys/logs`
  * `/var/vcap/packages`
    * Make sure the files are there
    * Make sure binaries are compiled for the right platform
    * Make sure permissions are right
  * `/var/vcap/jobs`
    * Make sure the start/stop scripts are there
    * Make sure the start script has the ERB template values replaced with the real value
    * Try stopping the job with the start script
  * Find the process id for the job (i.e. `ps -ef | grep number-picker`)
    * Look inside /proc/\<PID>.  `environ` is useful in seeing environment variables
    * validate this matched the PIDFILE

TODOs
-----

1. Add a smoke-test errand job to show errands
2. Pare down the deployment and runtime config manifests to remove any IAAS'y stuff that might be confusing.  I'll need to have some things in there, just to get the deployment to work, but I want to keep it as *lean* as possible.
3. Maybe write numbers to a disk to help describes using persistent volumes.
4. Maybe, eventually talk about building a tile using `tile-generator` and configuring through PCF Ops Manager?
5. ???
6. Profit