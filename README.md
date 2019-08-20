# AvP Forever

The AvP Forever project focuses on maintenance of available source code for
[Aliens versus Predator (1999)](https://pcgamingwiki.com/wiki/Aliens_versus_Predator)
game. The immediate goal is to create well-working Linux port suitable for
[Luxtorpeda project](https://github.com/dreamer/luxtorpeda/).

Branches named `other/*` contain various older ports and projects.


## Branches

- `other/icculus-releases`

  This branch contains snapshots of [icculus releases](https://icculus.org/avp/):

  - [`avp-20141225`](https://icculus.org/avp/files/avp-20141225.tar.gz)
  - [`avp-20150214`](https://icculus.org/avp/files/avp-20150214.tar.gz)
  - [`avp-20170505-a1`](https://icculus.org/avp/files/avp-20170505-a1.tar.gz)

  Content of tarballs was rebased on top of official Rebellion source code release
  (Build 117), preserving original git-blame information.

  Contains SDL1.2 based port and experimental SDL2 support (Linux, OS X).


- `other/avp-source-code-update`

  [Aliens versus Predator Source code update](http://homepage.eircom.net/~duncandsl/avp/)

  This branch contains code from
  [SVN repo](https://app.assembla.com/spaces/AvP/trac_subversion_tool) path
  [`^/branches/workbranch/trunk/`](http://subversion.assembla.com/svn/AvP/branches/workbranch/trunk/)
  imported to Git and rebased on top of official Rebellion source code release
  (Build 116).

  Contains port to Windows systems XP/Vista/7/8, Direct3D9 renderer, and
  numerous other fixes and improvements.


- `other/icculus-2009`

  The complete history of the icculus development branch until November 2009.

  Imported from [mbait/avpmp](https://github.com/mbait/avpmp) master branch,
  rebased on top of official Rebellion source code release (Build 116).


- `other/icculus-releases-rebased-1`

  icculus snapshots rebased on top of `other/icculus-2009`. Tip of this branch
  points to the same tree as `other/icculus-releases`, except the history of
  the branch is more detailed.


- `other/neuromancer/avp`
  
  Imported from [neuromancer/avp](https://github.com/neuromancer/avp)
  master branch, rebased on top of `other/icculus/release-rebased-1`.
 
  This branch includes few changes developed for
  [MorphOS release](http://aminet.net/package/game/shoot/avp.src-morphos),
  ported to icculus `avp-20170505-a1` release for Linux.

  Contains changes for FMV playback using `libav`.


- `other/scraft/avpmp`

  Imported from [Scraft/avpmp](https://github.com/Scraft/avpmp) master branch,
  rebased on top of `other/icculus-2009`.

  Contains changes to support OpenGL ES for
  [Pyra and OpenPandora](https://pyra-handheld.com/boards/pages/pyra/)
  handheld consoles.
