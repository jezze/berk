# Berk

## About

This program called berk is a simple but versatile job dispatcher. It can, given a set of machines, called remotes, execute jobs on each machine and log the outputs.

Jobs can be issued both synchronously or asynchronously, and can be monitored. Various statuses, stdout and stderr is logged and can be easily viewed at any point.

Berk was written to be a replacement for Jenkins and similar bloated software and uses a syntax and design similar to git.

It was written by Jens Nyberg.

## Build

Run:

    $ make

## Install

    $ sudo make install

## Package

Source:

    $ make dist

For Arch:

    $ cd pkgbuild
    $ make

For Debian:

    $ cd deb
    $ make

## Quick tutorial

### Initialization

First choose a directory where berk can store it's configuration. The recommended way is to use the root of your source tree which is the same directory where you typically can find the .git directory.

Then issue this command:

    $ berk init

This will create a .berk folder. If you later invoke berk from either here or from a subdirectory this is where berk will look for it's configuration.

### Remotes

Next we need to add a remote. A remote is a computer that we can connect to (usually over ssh) and tell it to execute jobs for us.

If you want to create a remote that has a running ssh server (which is the default) you can supply a hostname and a name for your machine:

    $ berk remote add -h "myhost.mydomain.com" "myhost"

Alternatively, if you don't have a computer with ssh you can create a remote that runs on your local computer:

    $ berk remote add -t local "mylocalhost"

To check what remotes you have you can do:

    $ berk remote

### Configure remotes

To check the configuration of "myhost" do:

    $ berk config "myhost"

For the remote, you might want a different username. Say you want the user to be called "testuser" instead you write:

    $ berk config set username "testuser" "myhost"

By default the private and public key pairs will be the id_rsa and id_rsa.pub keys located in ~/.ssh. If you want to use different keys you can configure those now as well. Berk should support most type of keys.

NOTE: Make sure that the public key you use already exist in the authorized_keys file for the desired user on the remote.

    $ berk config set privatekey "<path>/id_rsa" "myhost"
    $ berk config set publickey "<path>/id_rsa.pub" "myhost"

Instead of keys, you can set a password for the remote. If you set a password, it will take precedence over your ssh keys.

    $ berk config set password MySecretPassword myhost

NOTE: The password will be stored in clear text so only use this in an environment where you know its safe to do so.

### Executing jobs

Now it's time to actually execute a job on your remote. Since berk is mostly intended to operate on multiple remotes simultanously we are going to send the same command to multiple remotes.

This command will check the uptime on "myhost" twice: 

    $ berk exec -c "uptime" "myhost" "myhost"

NOTE: In normal circumstance you will typically use different remotes but since we only defined one, this will have to do. You can add more remotes using berk remote add.

Take notice of the job id that was printed. You can use the job id later to inspect the result of the job.

By default, berk will run a job against all remotes in parallell and asynchronously. There are flags that changes this behaviour:

    -n: Jobs will not fork and instead run sequentially and berk will exit when all jobs are done.
    -w: Berk will run everything in parallell but wait for everything to finish before exiting.

If you want to wait for the latest job to finish you can also run:

    $ berk wait

If a job takes to long or has become stuck you can kill it with:

    $ berk stop

Next, we will check on the results.

### Show information

Remember the job id you got from your the exec command in the previous section?

To look at the status of your job you can run:

    $ berk show <id>

But if you are only interested in the latest job you can skip the id, and you will get the same result:

    $ berk show

You can also use the special reference HEAD just like in git. You also do not need to supply the full id, Here are some examples:

    $ berk show HEAD
    $ berk show HEAD~1
    $ berk show HEAD HEAD~1
    $ berk show 8219a1c8
    $ berk show c78aa HEAD~12

To inspect all executions that have taken place you can run:

    $ berk log

Each remote of each execution will have it's own unique sequence number referred to as a run number. In our case you should see two runs called 0 and 1 because we previously used two remotes in our execution.

To look at the output for each remote, for the latest job, you can do:

    $ berk show -r 0 -o
    $ berk show -r 1 -o

By default 0 is used so you can just write this to see the first one:

    $ berk show -o

If a run for some reason failed you can instead look at the error output:

    $ berk show -e

### Transfer files

You can transfer files over to your remote:

    $ berk send "myfile.txt" "/home/myname/myfile.txt" "myhost"

You can send the same file to the same destination on multiple machines in one go as well by just adding more remotes at the end.

### Tags

To do more advanced setups you can tag remotes. This way you can for instance tell berk to only execute commands on remotes with a certain tag. On our remote we can add two tags called "beer" and "donut" using:

    $ berk config set tags "beer donut" "myhost"

Using berk remote with a tag we can now filter out only the remotes that match the tag we are interested in like "donut":

    $ berk remote -t donut

We can use the results to run our uptime command again but only on the remotes we previously filtered out:

    $ berk exec -c "uptime" $(berk remote -t donut)

This way, and with some bash magic, you can make very complex execution schemes.

### Shell

Another nice feature is that you can easily connect to your remote and get a shell:

    $ berk shell "myhost"

This is nice if you want to connect to your remote to do some changes.

### Hooks

Under .berk/hooks there are a bunch of samples. If you remove the .sample suffix from any of them they will get executed while berk is running.

    begin: Triggered before a job is started.
    end: Triggered after a job has finished.
    start: Triggered before a run on a remote is started.
    stop: Triggered after a run on a remote is finished.
    send: Triggered when a send is performed.

As an example, you can use these hooks to script your own notifications.
