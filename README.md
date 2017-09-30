What are these hacks anyway ?
=============================

I used to manage a wiki/blog website using the [Snip Snap][1] system
for an organization I am involved in. I did not choose this system
which was imposed to me due to time constraints (mother of all evils
:-). I considered that choice to be a really bad one for differents
reasons amongst which memory greediness and very poor documentation of
the storage data structure.

Once I was sur that the developpement ceased and after numerous
crashes due to memory lack on the hosting computer I decided to change
the wiki system to another one much more supported and documented (I
first choose [Ikiwiki][2] but then write my own system and finally
returned to Ikiwiki, for those who are interested). Unfortunately
since no good documentation of the content storage structure was
available this turns to be a nightmare.

I thus decided to document everything I can and tried to set up some
tools enabling one to get ride of this system without losing data
stored into it.

None of the code deliver here have been tested since a long time. Use
it at your own risks.

[1]: http://snipsnap.org/
[2]: http://ikiwiki.info


How to dump data stored in a SNIP-SNAP system
=============================================

For the following explanation I consider : `DEST=/tmp/snips`

1. Stop the snip-snap system on the host where it works

2. Remove `$HOME/.java/.userPrefs/org/snipsnap/`

3. Fix `$HOME/.java/.userPrefs/org/snipsnap/config.prefs.xml` using the
   template from this directory. The important part is the content of
   `$HOME/.java/.userPrefs/org/snipsnap/config/prefs.xml`, and in this file
   what is sensible is the `snipsnap.server.webapp.root` key.
   
   In the following example I put the value to be in `$DEST`.

```xml
<?xml version="1.0" encoding="UTF-8"?><!DOCTYPE map SYSTEM "http://java.sun.com/dtd/preferences.dtd">
<map MAP_XML_VERSION="1.0">
<entry key="snipsnap.server.admin.password" value="663b9"/>
<entry key="snipsnap.server.admin.rpc.url" value="http://localhost:8574"/>
<entry key="snipsnap.server.encoding" value="UTF-8"/>
<entry key="snipsnap.server.version" value="1.0b3-uttoxeter"/>
<entry key="snipsnap.server.webapp.root" value="/tmp/snips/applications"/>
</map>
```

4. Unzip the `snipsnap.tgz` file into `$DEST`

5. Copy the `_8668_` directory on the host you are working in `$DEST/applications`

6. Fix your `JAVA_HOME` variable

7. Call `$DEST/run.sh`

8. Login into `http://localhost:8668`

9. Export Database in SnipSnap Directory through the web interface

10. Copy the exported database where you want to work with it. The
    exported database is the file :
    
```
$DEST/applications/_8668_/webapp/WEB-INF/WIKI-DATE.snip`
```

What is stored in snipspace xml file ?
--------------------------------------

There are basically 2 kinds of elements :

* `<user>` containing :
  + login
  + passwd
  + email
  + roles
  + status
  + cTime
  + mTime
  + lastAccess
  + lastLogin
  + lastLogout
  + application

* `<snip>` containing :
  + name
  + oUser
  + cUser
  + mUser
  + cTime
  + mTime
  + permissions
  + backlinks
  + sniplinks
  + labels
  + attachments containing one <attachments/> or one to many
    <attachment> each containing :
    - name
    - content-type
    - size
    - date
    - location
    - data (base64 encoded)
  + viewCount
  + content
  + application
  + parentSnip
  + commentSnip

What is available here ?
------------------------

* [`snipexport.c`](snipexport.c)

  A small C program enabling you to export data stored in SnipSnap
  dump file into many files.

  You need `libxml2-dev` installed to compile it. As shown in the
  given [`makefile`](makefile), you need to adapt `CFLAGS` and
  `LDFLAGS` to compile it.

* [`snipsnap.tgz`](snipsnap.tgz)

  Last released version of Snip-Snap. That should now be available in a
  repository on github (https://github.com/thinkberg/snipsnap) but I, however,
  did not check if it's the same release.
