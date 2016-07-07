# About

This program called berk is a simple but versatile job dispatcher. It can,
given a set of remote machines, execute commands on each machine and log the
output.

It was written to be a replacement for Jenkins and similar bloated software
and uses a syntax and design similar to git.

It was written by Jens Nyberg.

# Getting started

First choose a directory where berk can store it's configuration. The
recommended way is to use the root of your source tree which is the same
directory where you typically can find the .git directory.

Then issue this command:

    $ berk init

This will create a .berk folder. If you later invoke berk from either here
or from a subdirectory this is where berk will look for it's configuration.

Next we need to add a remote. A remote is basically just a machine with an
ssh server running that we can contact and tell it to do work for us.

This command will add a new remote called "myhost":

    $ berk add myhost myhost.mydomain.com

To check what remotes you have you can do:

    $ berk list

To check the configuration of "myhost" do:

    $ berk config myhost

It is easy to change the configuration for a remote. Say you want the user to
be called "testuser" instead you can write:

    $ berk config myhost username testuser

Make sure the path to the SSH keys are correct. Also, make sure that the public
key exist in the authorized_keys file for the current user on each remote.

Now it's time to execute a command on your remote, but because berk is
intended to be used with many remotes simultanously we are gonna issue the
same command multiple times on the same remote.

    $ berk exec "myhost myhost" "uptime"

This command will check the uptime on the same remote twice. In normal
circumstance you will typically use different remotes. You can add more remotes
using berk add.

The output you see on screen is a set of events. You can hook into these
events in the same way as git in order to let berk perform specific tasks like
sending notifications. Check the samples in .berk/hooks/.

Take notice of the pid values for each start event. You can use this pid to
look at the output of the command you just executed on that remote.

    $ berk log <pid>

To do more advanced setups you can, using berk config, set labels seperated by
space, on each remote. This way you can tell berk to only execute commands on
remotes with a certain label. On our remote we can add two labels called "beer"
and "donut" using:

    $ berk config myhost label "beer donut"

Using berk list with a label we can now filter out only the machines that match
the label we are interested in like "donut":

    $ berk list donut

We can use the results to run our uptime command again but only on the remotes
we previously filtered out:

    $ berk exec "$(berk list donut)" "uptime"

This way, and with some bash magic, we can make very complex setups.
