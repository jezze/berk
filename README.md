# About

This program called berk is a simple but versatile job dispatcher. It can, given a set of machines, called remotes, execute jobs on each machine and log the outputs.

Jobs can be issued both synchronously or asynchronously, and can be monitored. Various statuses, stdout and stderr is logged and can be easily viewed at any point.

Berk was written to be a replacement for Jenkins and similar bloated software and uses a syntax and design similar to git.

It was written by Jens Nyberg.

# Build

Run:

$ make

# Install

$ sudo make install

# Package

Source:

$ make dist

For Arch:

$ cd pkgbuild
$ make

For Debian:

$ cd deb
$ make

# Getting started

First choose a directory where berk can store it's configuration. The recommended way is to use the root of your source tree which is the same directory where you typically can find the .git directory.

Then issue this command:

    $ berk init

This will create a .berk folder. If you later invoke berk from either here or from a subdirectory this is where berk will look for it's configuration.

Next we need to add a remote. A remote is basically just a machine with that we can contact and tell it to execute commands.

If you want to create a remote that talks ssh (which is the default) you can do:

    $ berk remote add -h "myhost.mydomain.com" "myhost"

Sometimes its nice to use your own machine as a remote, for testing purposes:

    $ berk remote add -t local "mylocalhost"

To check what remotes you have you can do:

    $ berk remote

To check the configuration of "myhost" do:

    $ berk config "myhost"

It is easy to change the configuration for a remote. Say you want the user to be called "testuser" instead you can write:

    $ berk config set username "testuser" "myhost"

By default the private and public key pairs will be the id_rsa and id_rsa.pub keys located in ~/.ssh. If you want to use different keys you can configure those now as well. Also, make sure that the public key you want to use exist in the authorized_keys file for the current user on each remote.

    $ berk config set privatekey "<path>/id_rsa" "myhost"
    $ berk config set publickey "<path>/id_rsa.pub" "myhost"

It is also possible, but not exactly recommended to set a password for the remote machine as well. This password will be stored in clear text so only use this in an environment where you know its safe to store data like that. Use this command to set a password:

    $ berk config password MySecretPassword myhost

Now it's time to execute a command on your remote. Since berk is intended to be used with many remotes simultanously we are gonna issue the same command multiple times on the same remote.

    $ berk exec "uptime" "myhost" "myhost"

This command will check the uptime on the same remote twice. In normal circumstance you will typically use different remotes. You can add more remotes using berk remote add.

Take notice of the id value that is printed. The id number is used to identify each call to berk exec. You can use the number to inspect the log for that particular id:

    $ berk show -i <id>

Or to just show the latest one, which will give you the same result:

    $ berk show

You can also use the special reference HEAD just like in git:

    $ berk show -i HEAD
    $ berk show -i HEAD~1

To inspect all executions that have taken place run:

    $ berk log

Each remote in each run will have it's own unique index number, referred to as a run, so in our case you should see two runs called 0 and 1 because we previously used two remotes in our execution.

To look at the output for each remote you can do:

    $ berk show -r 0 -o
    $ berk show -r 1 -o

By default 0 is used so you can just write:

    $ berk show -o

If a run for some reason failed you can instead look at the error output:

    $ berk show -e

To do more advanced setups you can, using berk config, set tags seperated by space, on each remote. This way you can tell berk to only execute commands on remotes with a certain tag. On our remote we can add two tags called "beer" and "donut" using:

    $ berk config set tags "beer donut" "myhost"

Using berk list with a tag we can now filter out only the remotes that match the tag we are interested in like "donut":

    $ berk remote -t donut

We can use the results to run our uptime command again but only on the remotes we previously filtered out:

    $ berk exec "uptime" $(berk list -t donut)

This way, and with some bash magic, you can make very complex execution schemes.
