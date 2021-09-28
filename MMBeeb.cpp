// MMBeeb Interface DLL
// August 2005 - Martin Mather
// http://mmbeeb.mysite.wanadoo-members.co.uk/
// Based in part on code by Richard Gellman

// This is not intended to be a full emulation of the MMC card,
// just enough for the patched DFS to work.

// Updated June 2019 to allow access to images > 2GB
// Updated September 2021 to compile in Visual Studio 2019

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <string.h>

#include "mmbeeb.h"

#define MMB_NAME "\\BeebEm\\DiscIms\\beeb.mmb"
#define CFG_NAME "\\BeebEm\\MMC.cfg"

static int DCAddress = 0xfe18;
static char MMBeebName[100] = "MMBeebXL Extension board for BBC Model B";
static char MMBImageName[MAX_PATH];
static char MMBFileName[MAX_PATH] = MMB_NAME;

enum MMCprocess {
        MMCidle = 1,
        MMCcmd0 = 2,
        MMCcmd1 = 3,
        MMCreaddata = 4,
        MMCwritedata = 5,
        MMCcardid = 6,
        MMCsetblklen = 7
};

enum MMCstatus {
        MMCnogo = 0,
        MMCcmd0ok = 1,
        MMCcmd1fail = 2,
        MMCcmd1ok = 3
};

static unsigned char shiftreg = 0xff;

// Card ID - I'm familiar with these numbers
static unsigned char CardID[] = {
        0xff, 0xfe, 0x01, 0x00, 0x00,
        0x33, 0x32, 0xff, 0xff, 0xff,
        0xff, 0xff, 0x19, 0x88, 0x4f,
        0x7a, 0x34, 0xff, 0x6a, 0xca
};

static const unsigned long BufferSize = 0x200;
static unsigned char Buffer[BufferSize];
static unsigned long BufferAddress;
static BOOL BufferEmpty = TRUE;

static unsigned char MMCprocess = MMCidle;
static unsigned char MMCstatus = MMCnogo;
static unsigned int MMCcounter = 0;
static unsigned long MMCaddress = 0;
static unsigned long MMCaddresslimit = 0;
static unsigned int MMCblocklength = 0x0200;
static const unsigned int MMCwritelength = 0x0200;

static BOOL DoSetup = TRUE;

// Write MMC
EXPORT unsigned char SetDriveControl(unsigned char value) {
        writeMMC (value);
        return 0;
}

// Read MMC
EXPORT unsigned char GetDriveControl(unsigned char value) {
        return shiftreg;
}

// Beebem seems to call this twice when initialising
EXPORT void GetBoardProperties(struct DriveControlBlock *FDBoard) {
        // Initialise code
        if (DoSetup == TRUE) {
            SetAddressLimit();
            DoSetup=FALSE;
        }

        sprintf(MMBeebName, "MMBeebXL: %.50s %04X", MMBFileName, DCAddress);

        FDBoard->FDCAddress = 0x0000; // Not used
        FDBoard->DCAddress = DCAddress; // MMC Board
        FDBoard->BoardName = MMBeebName;
        FDBoard->TR00_ActiveHigh = TRUE;
}

// MMC code
void writeMMC(unsigned char value) {
        shiftreg = 0xff;
        MMCcounter++;

        // Read command address
        if (MMCprocess != MMCidle && MMCcounter < 6) {
            MMCaddress = (MMCaddress << 8) | value;
            return;
        }

        switch (MMCprocess) {
            case MMCidle: // Wait for valid command
                MMCcounter = 1;
                MMCaddress = 0;
                if (value == 0x40 && MMCaddresslimit > 0) {
                    MMCstatus = MMCnogo;
                    MMCprocess = MMCcmd0;
                } else if (MMCstatus >= MMCcmd0ok) {
                    if (value == 0x41)
                        MMCprocess = MMCcmd1;
                    else if (MMCstatus == MMCcmd1ok) {
                        switch (value) {
                            case 0x4a:
                                MMCprocess = MMCcardid;
                                break;
                            case 0x50:
                                MMCprocess = MMCsetblklen;
                                break;
                            case 0x51:
                                MMCprocess = MMCreaddata;
                                break;
                            case 0x58:
                                MMCprocess = MMCwritedata;
                                break;
                        }
                    }
                }
                break;
            case MMCreaddata: // Read data block
                if (MMCcounter > 9) {
                    if (MMCcounter < (MMCblocklength + 10))
                        shiftreg = Buffer[MMCcounter - 10];
                    else if (MMCcounter == (MMCblocklength + 12))
                        MMCprocess = MMCidle;
                } else if (MMCcounter == 9)
                    shiftreg = 0xfe;
                else if (MMCcounter == 8) {
                    if (MMCaddress < MMCaddresslimit) {
                        // read sector from file
                        ReadSector();
                        if (BufferEmpty) {
                            shiftreg = 0xff; // read error?
                            MMCprocess = MMCidle;
                        } else
                            shiftreg = 0x00;
                    } else {
                        shiftreg = 0x40;
                        MMCprocess = MMCidle;
                    }
                }
                break;
            case MMCwritedata: // Write data block
                if (MMCcounter > 11) {
                    if (MMCcounter < (MMCwritelength + 12))
                        Buffer[MMCcounter - 12] = value;
                    else if (MMCcounter == (MMCwritelength + 14)) {
                        shiftreg = WriteSector();
                        MMCprocess = MMCidle;
                    }
                } else if (MMCcounter == 8) {
                    if ((MMCaddress & 0x1ff) != 0) { // Must be aligned to sector
                        shiftreg = 0x40;
                        MMCprocess = MMCidle;
                    } else
                        shiftreg = 0x00;
                }
                break;
            case MMCcmd0: // CMD 0 (Reset card)
                if (MMCcounter == 8) {
                    shiftreg = 0x01;
                    MMCstatus = MMCcmd0ok;
                    MMCprocess = MMCidle;
                }
                break;
            case MMCcmd1: // CMD 1 (Initialise card)
                if (MMCcounter == 8) {
                    if (MMCstatus == MMCcmd0ok) {
                        shiftreg = 0x01;
                        MMCstatus = MMCcmd1fail;
                    } else {
                        shiftreg = 0x00;
                        MMCstatus = MMCcmd1ok;
                    }
                    MMCprocess = MMCidle;
                }
                break;
            case MMCcardid: // Read Card ID
                if (MMCcounter == 8) {
                    shiftreg = 0x00;
                } else if (MMCcounter > 8) {
                    if (MMCcounter == 28) {
                        MMCprocess = MMCidle;
                    } else {
                        shiftreg = CardID[MMCcounter - 9];
                    }
                }
                break;
            case MMCsetblklen: // Set (Read) Block Length
                if (MMCcounter == 8) {
                    if (MMCaddress != 0 && MMCaddress <= BufferSize) {
                        shiftreg = 0x00;
                        MMCblocklength = MMCaddress;
                    } else
                        shiftreg = 0x40;
                    MMCprocess = MMCidle;
                }
                break;           
        }
}

// Return size of image
void SetAddressLimit() {
        FILE * ImageFile;
        FILE * pFile;
        char buf[MAX_PATH + 50];
   
        MMCaddresslimit=0x0;

        // MMB file in Documents folder?
        HRESULT result = SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, MMBImageName);
        int x = strlen(MMBImageName);

        // Optional mmc.cfg file
        // containing path to image file, and/or device address
        strcpy(MMBImageName + x, CFG_NAME);
        pFile = fopen(MMBImageName,"rt");
        if (pFile!=NULL)
        {
            int i, b = fread (buf, 1, sizeof(buf), pFile);
            fclose (pFile);
            if (b > 0) {
                // First line is pathname
                for (i = 0; buf[i] >= 0x20; i++);
                buf[i] = 0;

                if (i > 0)
                     strcpy(MMBFileName, buf);

                // Second line is address
                for (; i < b - 1; i++)
                    if (buf[i] == '1' && buf[i + 1] == 'C')
                        DCAddress = 0xfe1c;
            }
        }

        if (MMBFileName[0] != '\\')
            x = 0;

        strcpy(MMBImageName + x, MMBFileName);

        // Check if image exists & get its size
        ImageFile=fopen(MMBImageName, "rb");
        if (ImageFile != NULL) {
            fseek (ImageFile, 0, SEEK_END);
            MMCaddresslimit = ftell(ImageFile);
            fclose (ImageFile);
        } else {
            // Report image not found
            sprintf (buf, "Can't find image: %s", MMBImageName);
            MessageBox (0, buf, "MMB", MB_OK|MB_ICONERROR);
        }
}

// Read data into buffer from image
void ReadSector() {
        int itemsread;
        FILE *ImageFile;

        if (BufferEmpty | (MMCaddress != BufferAddress)) {
            BufferEmpty = TRUE;

            ImageFile = fopen(MMBImageName, "rb");
            if (ImageFile != NULL) {
                //fseek (ImageFile, MMCaddress, SEEK_SET);
                myfseek (ImageFile, MMCaddress);
                // Note; BufferSize >= MMCblocklength
                itemsread = fread (Buffer, BufferSize, 1, ImageFile);
                fclose (ImageFile);
                if (itemsread == 1) {
                    BufferAddress = MMCaddress;
                    BufferEmpty = FALSE;
                }   
            }
        }
}

// Write data from buffer to image
unsigned char WriteSector() {
        int itemswritten;
        FILE *ImageFile;

        BufferEmpty = TRUE;
        
        ImageFile = fopen(MMBImageName, "r+b");
        if (ImageFile != NULL) {
            //fseek (ImageFile, MMCaddress, SEEK_SET);
            myfseek (ImageFile, MMCaddress);
            itemswritten = fwrite (Buffer, MMCwritelength, 1, ImageFile);
            fclose (ImageFile);
            if (itemswritten == 1)
            {
                BufferAddress = MMCaddress;
                BufferEmpty = FALSE;         
                return 0x05;
            }
        }
       
        return 0xff;
}           

// Set file pointer (18/06/2019)
// This deals with images > 2GB
int myfseek(FILE *stream, unsigned long offset) { 
        const unsigned long skip = 0x7FFFFFFE;
        int ret;

        ret = fseek(stream, 0, SEEK_SET);
        if (ret == 0) {
            do {
                if (offset <= skip) {
                    ret = fseek(stream, offset, SEEK_CUR);
                    offset = 0;
                } else {
                    ret = fseek(stream, skip, SEEK_CUR);
                    offset -= skip;
                }
            } while (offset != 0 && ret == 0);
        }
                
        return ret;
}

