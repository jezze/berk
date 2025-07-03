# About

This program called berk is a simple but versatile job dispatcher. It can,
given a set of machines, called remotes, execute commands on each machine and
log the outputs.

It was written to be a replacement for Jenkins and similar bloated software
and uses a syntax and design similar to git.

It was written by Jens Nyberg.

# Getting started

First choose a directory where berk can store it's configuration. The
recommended way is to use the root of your source tree which is the same
directory where you typically can find the .git directory.

Then issue this command:

    $ berk init

This will create a .berk folder. If you later invoke berk from either here or
from a subdirectory this is where berk will look for it's configuration.

Next we need to add a remote. A remote is basically just a machine with a
running ssh server that we can contact and tell it to execute commands.

This command will add a new remote called "myhost":

    $ berk add myhost -h myhost.mydomain.com

To check what remotes you have you can do:

    $ berk list

To check the configuration of "myhost" do:

    $ berk config myhost

It is easy to change the configuration for a remote. Say you want the user to
be called "testuser" instead you can write:

    $ berk config myhost username testuser

By default the private and public key pairs will be the id_rsa and id_rsa.pub
keys located in ~/.ssh. If you want to use different keys you can configure
those now as well. Also, make sure that the public key you want to use exist in
the authorized_keys file for the current user on each remote.

    $ berk config myhost privatekey id_rsa
    $ berk config myhost publickey id_rsa.pub

It is also possible, but not exactly recommended to set a password for the
remote machine as well. This password will be stored in clear text so only use
this in an environment where you know its safe to store data like that. Use
this command to set a password:

    $ berk config myhost password MySecretPassword

Now it's time to execute a command on your remote. Since berk is intended to be
used with many remotes simultanously we are gonna issue the same command
multiple times on the same remote.

    $ berk exec "myhost myhost" "uptime"

This command will check the uptime on the same remote twice. In normal
circumstance you will typically use different remotes. You can add more remotes
using berk add.

The output you see on screen is a set of events. You can hook into these
events in the same way as git in order to let berk perform specific tasks like
sending notifications. Check the samples in .berk/hooks/.

Take notice of the id value for the begin event. The id number is used to
identify each call to berk exec. You can use the number to inspect the log for
that particular run:

    $ berk log <id>

Each remote in each run will have it's own unique index number called a run so
in our case you should see a 0 and 1 because we previously used two remotes in
our execution. To look at the output for each remote you can do:

    $ berk log <id> 0
    $ berk log <id> 1

To do more advanced setups you can, using berk config, set tags seperated by
space, on each remote. This way you can tell berk to only execute commands on
remotes with a certain tag. On our remote we can add two tags called "beer"
and "donut" using:

    $ berk config myhost tags "beer donut"

Using berk list with a tag we can now filter out only the remotes that match
the tag we are interested in like "donut":

    $ berk list -t donut

We can use the results to run our uptime command again but only on the remotes
we previously filtered out:

    $ berk exec "$(berk list -t donut)" "uptime"

This way, and with some bash magic, we can make very complex setups.
