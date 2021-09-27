// MMBeeb Interface DLL
// August 2005 - Martin Mather
// http://mmbeeb.mysite.wanadoo-members.co.uk/
// Based in part on code by Richard Gellman

// Updated September 2021 to compile in Visual Studio 2019


#ifdef __cplusplus
#define EXPORT extern "C" __declspec (dllexport)
#else
#define EXPORT __declspec (dllexport)
#endif

#define WIN_LEAN_AND_MEAN

struct DriveControlBlock {
        int FDCAddress; // FDC chip address (not used)
        int DCAddress; // MMC interface address
        char *BoardName;
        bool TR00_ActiveHigh;
};

EXPORT unsigned char SetDriveControl(unsigned char value);
EXPORT unsigned char GetDriveControl(unsigned char value);
EXPORT void GetBoardProperties(struct DriveControlBlock *FDBoard);

void writeMMC(unsigned char value);
void SetAddressLimit();
void ReadSector();
unsigned char WriteSector();
int myfseek(FILE *stream, unsigned long offset);


