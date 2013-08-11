racka3
======

Racka3 (pronounced rackarackarack, or not) aims to be a standalone guitar-orientated multi-effects solution. It is
different from the existing (and very excellent) programs that exist for this purpose because of the following:

Standalone
----------

Racka3 is a command-line application, with no dependencies on GUI toolkits. All GUI toolkits have problems, and
restrict GUI development to those people who can work that specific code style. This is why many open source works have
bad GUIs.

Because it is standalone, Racka3 can be embedded more easily, or be made bootable. This is fun because we can cook a 
tight and slim linux distro that boots and runs Racka3 - getting the most out of old hardware so allowing us to run Racka3
on an ol laptop or whatever. It also gives a good base for creating a real hardware guitar pedal.

So... No GUI?
-------------

Well, not really. Racka3 shall come with a GUI, but this exists as HTML, which is served by a webserver built into Racka3.
Sounds crazy? Not at all! HTML, with some Javascripting (AJAX, if you must) has many benefits:
* We can splendid GUIs in HTML very easily
* Enforces decoupling of GUI and engine
* Allows local or remote GUI (for example GUI can be shown on smartphone)

Interface
---------

Racka3 hosts the HTML using a built-in webserver, which also presents a RESTful-ish interface by which commands are sent
to the engine from the GUI, and the engine can be queried. We use JSON because it is natural for javascript and frankly
less arseache than XML. This will allow people to create embedde interfaces as well, as JSON is easy to speak.

Instructions
------------

Lets walk before we run. I will write these when I have written the code.

Shout-outs
----------

Rakkarack
cJSON library
UTHASH
Mongoose webserver
JQuery Knob - Anthony Terrien - http://anthonyterrien.com/knob/
HTML5 sotable  - Ali Farhadi- http://farhadi.ir/projects/html5sortable/

