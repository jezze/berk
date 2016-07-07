# About

This program called berk is a simple but versatile job dispatcher. It can,
given a set of remote machines, execute commands on each machine and log the
output.

It was written to be a replacement for Jenkins and similar bloated software
and uses a syntax and design similar to git.

It was written by Jens Nyberg.

# Getting started

First choose a directory where to store your berk configuration. The
recommended directory is the root of your source tree, the same directory
where you usually also find your .git directory.

Then issue this command:

    $ berk init

This will create a .berk folder. If you invoke berk from here or from a
subdirectory this is where berk will look for it's configuration.

Next we need to add a remote. A remote is basically just a machine that we
will later will execute commands we issue to it.

This command will create a new remote called 'myhost':

    $ berk add myhost myhost.mydomain.com

To check what remotes you have you can do:

    $ berk list

To check the settings of the myhost remote do:

    $ berk config myhost

It is easy to change settings for this remote. Say you want the login user to
be called testuser instead you can write:

    $ berk config myhost username testuser

Make sure the path to the SSH keys are correct. Also, make sure that the public
key exist in the authorized_keys file for the current user on each remote.

Now it's time to execute a command on your remote, but because berk is
intended to be used with many remotes simultanously we are gonna issue the
same command multiple times on the same remote.

    $ berk exec "myhost myhost" "uptime"

This command will check the uptime on the same remote twice. Usually you want
these to be different machines. You can add more remotes using berk add and try
it out.

The output you see on screen is a set of events. You can hook into these
events in the same way as git in order to let berk perform specific tasks like
sending notifications. Check .berk/hooks/.

Take noticed of the pid values for each start event. You can use this pid to
look at the output of the command you just executed on that remote.

    $ berk log <pid>

To do more advanced setups you can, using berk config, set space seperated
labels for each remote. In that way you can tell berk to only execute commands
on all remotes with a certain label. On our remote we can add two labels called
"beer" and "remote" using:

    $ berk config myhost label "donut beer"

Using berk list and adding a label we can now filter out only the machines that
matches a certain label like:

    $ berk list donut

We can use this information to run our uptime command again but only on remotes
with the label "donut":

    $ berk exec "$(berk list donut)" "uptime"

This way, or using even more bash magic, you are able to make very complex setups.
