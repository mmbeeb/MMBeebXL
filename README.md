# MMBeebXL.dll
MMBeebXL.dll allows the memory mapped version of MMFS to be used with BeebEm when emulating the Model B.

It should be placed in the Program Files (x86)\BeebEm\Hardware folder, and selected in the BeebEm menu Hardware>Model B Floppy Controller>Select FDC Board.

By default the pathname of the image file is Documents\BeebEm\DiscIms\beeb.mmb.

An optional mmc.cfg file can be created in Documents\BeebEm folder:

The first line can contain an alternative image pathname.  If it starts with '\\' then the Documents path will be prefixed.

The hardware address is now 0xfe1c.  This is the new default for MMFS (currently version 1.52).
