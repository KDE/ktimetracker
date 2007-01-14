#!/bin/bash

# Example of how to use xautomation.  
#
# Notes:
#
#   - This test fails for korg, as it opens a modal dialog box, even
#     after changes are saved (ref bug #94537).
#
#   - The xte str command is broken (it types to fast).  To be safe,
#     put in characters one-by-one using the key command.  Writing a 
#     bash function that does this for a given string would be helpful.
#
#   - This script uses hardcoded English short cut keys.  To make generic:
#
#       1. Get two char language code from OS (or KDE?)
#
#       2. Source bash script for language (e.g. source __shortcuts.en)
#
#       3. Call functions from that script (e.g. open_file $FILENAME) 
#
#   - Using killall isn't great.  karm has a quit() dcop function for clean
#     shutdown.  Need to check if korgac or korganizer provide this interface.


# kill any running processes
killall korganizer
killall korgac

# start korganizer
korganizer&

# make sure it's done opening
sleep 10

# open file in korganizer.
#   Note: this opens a second korg instance
xte 'keydown Alt_L'
xte 'key F'
xte 'keyup Alt_L'
xte 'key O'

# wait for open file dialog to come up
sleep 1

# put focus on file name control
xte 'keydown Alt_L'
xte 'key L'
xte 'keyup Alt_L'
xte 'key ~'
xte 'key /'
xte 'key t'
xte 'key e'
xte 'key s'
xte 'key t'
xte 'key .'
xte 'key i'
xte 'key c'
xte 'key s'

# save new storage file
xte 'keydown Alt_L'
xte 'key O'
xte 'keyup Alt_L'

sleep 1

# open new to-do dialog
xte 'keydown Alt_L'
xte 'key A'
xte 'keup Alt_L'
xte 'key v'

sleep 1

# set focus to title
xte 'keydown Alt_L'
xte 'key I'
xte 'keyup Alt_L'

# type in test task name (you have to type slowly for xte)
xte 'key T'
xte 'key e'
xte 'key s'
xte 'key t'
xte 'key -'
xte 'key t'
xte 'key a'
xte 'key s'
xte 'key k'
xte 'key -'
xte 'key 1'

sleep 1

# save new todo
xte 'keydown Alt_L'
xte 'key O'
xte 'keyup Alt_L'

sleep 1

# save changes to file
xte 'keydown Alt_L'
xte 'key F'
xte 'keyup Alt_L'
xte 'key S'

sleep 1

#   Quit below fails.
#
#   korg pops up a dialog that says:
#   
#     The calendar contains unsaved changes.
#     Do you want to save them before exiting?

# quit korganizer
xte 'keydown Control_L'
xte 'key q'
xte 'keyup Control_L'

# quit first korganizer view
xte 'keydown Control_L'
xte 'key q'
xte 'keyup Control_L'

sleep 1

killall korgac

# 1. cleanup
rm "~/test.ics"

