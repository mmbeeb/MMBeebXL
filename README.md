# MMBeebXL.dll
MMBeebXL.dll allows the memory mapped version of MMFS to be used with BeebEm when emulating the Model B.

It should be placed in the Program Files (x86)\BeebEm\Hardware folder, and selected in the BeebEm menu Hardware>Model B Floppy Controller>Select FDC Board.

By default the pathname of the image file is Documents\BeebEm\DiscIms\beeb.mmb.

An optional mmc.cfg file can be created in Documents\Beebem folder:

The first line can be blank or contain an alternative image pathname.  If it starts with '\' then the Documents path will be prefixed.
A second line allows a different 'hardware address' to be given, though only FE1C is valid, else the default FE18 is selected.

If the Econet is enabled, the address FE18 causes a conflict and the MMC won't work.  Untick Econet On/Off in the Hardware menu.
To use the FE1C address, you need a version "Model B MMFS MM" which has been changed to use this address.
