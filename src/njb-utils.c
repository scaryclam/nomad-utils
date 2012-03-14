#include <libnjb.h>
#include <sqlite3.h>
#include <string.h>

// Some prototypes
int detect(njb_t *njbs);
int get_devices(njb_t *njbs, int i);
int sync_db(njb_t *njb);
static void datafile_dump (njb_datafile_t *df, FILE *fp);
njb_songid_frame_t* songid_frame_find(njb_songid_t *tag, const char *);


static void datafile_dump(njb_datafile_t *df, FILE *fp)
{
    u_int64_t size = df->filesize;

    fprintf(fp, "File ID  : %u\n", df->dfid);
    fprintf(fp, "Filename : %s\n", df->filename);
    if (df->folder != NULL) {
        fprintf(fp, "Folder   : %s\n", df->folder);
    }
    fprintf(fp, "Fileflags: %08X\n", df->flags);
    #ifdef __WIN32__
        fprintf(fp, "Size :     %I64u bytes\n", size);
    #else
        fprintf(fp, "Size :     %llu bytes\n", size);
    #endif
}

static void songid_frame_dump (njb_songid_frame_t *frame, FILE *fp)
{
  fprintf(fp, "%s: ", frame->label);

  if ( frame->type == NJB_TYPE_STRING ) {
    fprintf(fp, "%s\n", frame->data.strval);
  } else if (frame->type == NJB_TYPE_UINT16) {
    fprintf(fp, "%d\n", frame->data.u_int16_val);
  } else if (frame->type == NJB_TYPE_UINT32) {
    fprintf(fp, "%u\n", frame->data.u_int32_val);
  } else {
    fprintf(fp, "(weird data word size, cannot display!)\n");
  }
}

njb_songid_frame_t* songid_frame_find(njb_songid_t *tag, const char *type)
{
    njb_songid_frame_t *frame;
    frame = NJB_Songid_Findframe(tag, type);
    return frame;
}

int sync_db(njb_t *njb)
{
    sqlite3 *db;
    int rc;
    int str_len;
    int result;
    char *zErrMsg = 0;
    char sql_statement[200];
    char *sql_statement_check;
    njb_songid_frame_t *frame;
    njb_songid_t *songtag;

    printf("Opening database\n");
    rc = sqlite3_open("njb.db", &db);
    if (rc)
    {
        /* Oh no, there was a problem opening the DB */
        fprintf(stderr, "Cannot open the database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }

    if (NJB_Open(njb) == -1)
    {
        NJB_Error_Dump(njb,stderr);
        return 1;
    }

    if (NJB_Capture(njb) == -1)
    {
        NJB_Error_Dump(njb,stderr);
        return -1;
    }

    printf("Looping through songs\n");
    NJB_Reset_Get_Track_Tag(njb);

    while ((songtag = NJB_Get_Track_Tag(njb)))
    {
        NJB_Songid_Reset_Getframe(songtag);
        /*
        songtag->trid
        */
        /*while ((frame = NJB_Songid_Getframe(songtag)))
        {
            printf("---------------------------------------\n");
        } */
        frame = songid_frame_find(songtag, FR_GENRE);
//        fprintf(stdout, "%s: ", frame->label);

//        if (frame->type == NJB_TYPE_STRING) {
//            fprintf(stdout, "%s\n", frame->data.strval);
//        } else if (frame->type == NJB_TYPE_UINT16) {
//            fprintf(stdout, "%d\n", frame->data.u_int16_val);
//        } else if (frame->type == NJB_TYPE_UINT32) {
//            fprintf(stdout, "%u\n", frame->data.u_int32_val);
//        } else {
//            fprintf(stdout, "(weird data word size, cannot display!)\n");
//        }

        strcpy(sql_statement, "SELECT * FROM genre WHERE name=");
        strncat(sql_statement, frame->data.strval, strlen(frame->data.strval));
        result = sqlite3_exec(db, sql_statement, 0, NULL, &zErrMsg);
        //printf("Result: %s", zErrMsg);

        if (zErrMsg)
        {
            sqlite3_free(zErrMsg);
            strcpy(sql_statement, "INSERT INTO genre (name) VALUES('");
            strncat(sql_statement, frame->data.strval, strlen(frame->data.strval));
            strncat(sql_statement, "')", 2);
            printf("Statement: %s\n", sql_statement);
            rc = sqlite3_exec(db, sql_statement, 0, NULL, &zErrMsg);
            if (rc != SQLITE_OK)
            {
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }
            NJB_Songid_Destroy(songtag);
        }
    }
    sqlite3_close(db);
    NJB_Release(njb);
    NJB_Close(njb);
    return 0;
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

    if ( NJB_Open(njb) == -1 ) {
        NJB_Error_Dump(njb, stderr);
        return 1;
    }

    if ( NJB_Capture(njb) == -1 ) {
        NJB_Error_Dump(njb, stderr);
        return 1;
    }

    printf("Getting datafile tag\n");
    NJB_Reset_Get_Datafile_Tag(njb);
    printf("Entering while loop to get ifo on all files\n");
    while ((filetag = NJB_Get_Datafile_Tag(njb))) {
        datafile_dump(filetag, stdout);
        NJB_Get_File(njb, filetag->dfid, filetag->filesize, filetag->filename,
NULL, NULL);
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
        sync_db(njbs);
    }

    return 0;
}

