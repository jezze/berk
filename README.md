# ABOUT

This program called berk is a simple but versatile job dispatcher. It can,
given a set of machines, execute commands on each machine and log the output.
It was written to be a replacement for Jenkins and similar bloated software.

It was written by Jens Nyberg.

# GETTING STARTED

Chose a folder where to store your configuration. It is recommended that you
stand in the same folder as you have your .git configuration in your source.

    $ berk init

This will create a .berk folder. If you invoke berk from here or from a
subdirectory this is where berk will look for it's configuration.

Next we need to add a machine called 'mymachine':

    $ berk add mymachine mymachine.mydomain.com

To check what machines you have you can do:

    $ berk list

To check the settings of the machine do:

    $ berk config mymachine

It is easy to change settings for this machine. Say you want the login user to
be called testuser instead you can write:

    $ berk config mymachine username testuser

Make sure the path to the SSH keys are correct. Also, make sure that on the
target machine the public key is available in the user's authorized_keys file.

Now it's time to execute a command on your machine, but because berk is
intended to be used with many machines simultanously we are gonna issue the
same command multiple times for the same machine.

    $ berk exec "mymachine mymachine" "uptime"

This command will check the uptime on a set of machines. In this command they
are the same machine but you can of course add more using berk add.

The output you see on screen is a set of events. You can hook into these
events in the same way as git in order to let berk perform specific tasks like
sending notifications.

Take noticed of the pid values for each start event. You can use this pid to
look at the output of the command you just executed on that machine.

    $ berk log <pid>

To do more advanced setups you can, using berk config, set space seperated
labels for each machine. In that way you can tell berk to only execute commands
on all machines with a certain label. For instance to only run the uptime
command on machines that have the label 'donut' you can do:

    $ berk exec "$(berk list donut)" "uptime"

This way you can make very complex machine setups.

