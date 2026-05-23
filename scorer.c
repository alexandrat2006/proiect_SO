#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int report_id;
    char inspector[32];
    double lat, lon;
    char category[20];
    int severity;
    time_t timestamp;
    char description[128];
} Report;

int main(int argc, char *argv[]) {
    if (argc < 2) return 1;
    char path[256];
    sprintf(path, "%s/reports.dat", argv[1]);

    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("Could not open reports for district %s\n", argv[1]);
        fflush(stdout);
        return 1;
    }

    struct { char name[32]; int total; } scores[50];
    int count = 0;
    Report r;

    while (fread(&r, sizeof(Report), 1, f)) {
        r.inspector[31] = '\0';
        int found = -1;
        for(int i=0; i<count; i++) {
            if(strcmp(scores[i].name, r.inspector) == 0) {
                found = i; 
                break;
            }
        }
        if(found != -1) {
            scores[found].total += r.severity;
        } else if(count < 50) {
            strncpy(scores[count].name, r.inspector, 31);
            scores[count].name[31] = '\0';
            scores[count].total = r.severity;
            count++;
        }
    }

    for(int i=0; i<count; i++) {
        printf("  - %s: %d points\n", scores[i].name, scores[i].total);
    }
    fflush(stdout);

    fclose(f);
    return 0;
}
