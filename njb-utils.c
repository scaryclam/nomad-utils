#include <libnjb.h>

// Some prototypes
int detect(njb_t *njbs);
int get_devices(njb_t *njbs, int i);
static void datafile_dump (njb_datafile_t *df, FILE *fp);


static void datafile_dump(njb_datafile_t *df, FILE *fp)
{
    long long unsigned int size = df->filesize;
  
    fprintf(fp, "File ID  : %u\n", df->dfid);
    fprintf(fp, "Filename : %s\n", df->filename);
    if (df->folder != NULL) {
        fprintf(fp, "Folder   : %s\n", df->folder);
    }
    fprintf(fp, "Fileflags: %08X\n", df->flags);
    fprintf(fp, "Size :     %llu bytes\n", size);
}

int detect(njb_t *njbs)
{
    int someint;

    fprintf(stderr, "Scanning for devices\n");
    if (NJB_Discover(njbs, 0, &someint) == -1) {
        fprintf(stderr, "No Jukebox Devices Found\n");
        return -1;
    }

    if (someint == 0) {
        fprintf(stderr, "Could not find any devices\n");
        return -1;
    }
    return someint;
}

int get_devices(njb_t *njbs, int i)
{
    njb_t *njb;
    njb = njbs;
    njb_datafile_t *filetag;

    printf("Getting Devices, opening device ...\n");
    if ( NJB_Open(njb) == -1 ) {
        NJB_Error_Dump(njb, stderr);
        return 1;
    }

    printf("Capturing the device\n");
    if ( NJB_Capture(njb) == -1 ) {
        NJB_Error_Dump(njb, stderr);
        return 1;
    }

    NJB_Reset_Get_Datafile_Tag(njb);
    while ( (filetag = NJB_Get_Datafile_Tag (njb)) ) {
        datafile_dump(filetag, stdout);
        printf("----------------------------------\n");
        NJB_Datafile_Destroy(filetag);
    }

    // Dump any pending errors
    if (NJB_Error_Pending(njb)) {
        NJB_Error_Dump(njb, stderr);
    }

    NJB_Release(njb);
    NJB_Close(njb);
}

int main(void)
{
    njb_t njbs[NJB_MAX_DEVICES];
    njb_t *njb;
    njb_keyval_t *key;
    int j;
    int i;
    int num_devs;
    int user_input;
    int chargestat;
    int auxpowstat;
    const char *devname;
    const char *prodname;

    fprintf(stdout, "Starting to scan\n");
    num_devs = detect(njbs);
    if (num_devs == -1) {
        fprintf(stderr, "Exiting\n");
        return 1;
    }

    for (i = 0; i < num_devs; i++) {
        njb = &njbs[i];

        printf("Device %u: ", i);
        printf("\n\tPlayer device type: ");
        njb = &njbs[i];

        if (NJB_Open(njb) == -1) {
            NJB_Error_Dump(njb, stderr);
            return 1;
        }

        if ((devname = NJB_Get_Device_Name(njb, 0)) != NULL) {
            printf("%s\n", devname);
        } else {
            printf("Error getting device name.\n");
            return 1;
        }

        NJB_Release(njb);
        NJB_Close(njb);
    }

    if (num_devs > 0) {
        printf("Please pick one from the above: ");
        scanf("%d", &user_input);
        get_devices(njbs, user_input);
    }

    return 0;
}

