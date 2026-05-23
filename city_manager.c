#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

typedef struct {
    int report_id;
    char inspector[32];
    double lat;
    double lon;
    char category[20];
    int severity;
    time_t timestamp;
    char description[128];
} Report;

void get_permissions_string(mode_t mode, char *str) {
    str[0] = (mode & S_IRUSR) ? 'r' : '-';
    str[1] = (mode & S_IWUSR) ? 'w' : '-';
    str[2] = (mode & S_IXUSR) ? 'x' : '-';
    str[3] = (mode & S_IRGRP) ? 'r' : '-';
    str[4] = (mode & S_IWGRP) ? 'w' : '-';
    str[5] = (mode & S_IXGRP) ? 'x' : '-';
    str[6] = (mode & S_IROTH) ? 'r' : '-';
    str[7] = (mode & S_IWOTH) ? 'w' : '-';
    str[8] = (mode & S_IXOTH) ? 'x' : '-';
    str[9] = '\0';
}

int check_permission(struct stat *st, const char *role, int perm) {
    if (strcmp(role, "manager") == 0) {
        return (st->st_mode & perm) ? 1 : 0;
    }
    if (strcmp(role, "inspector") == 0) {
        if (perm & S_IRUSR) if (!(st->st_mode & S_IRGRP)) return 0;
        if (perm & S_IWUSR) if (!(st->st_mode & S_IWGRP)) return 0;
        if (perm & S_IXUSR) if (!(st->st_mode & S_IXGRP)) return 0;
        return 1;
    }
    return 0;
}

void log_action(const char *dist, const char *user, const char *role, const char *msg) {
    char path[256];
    sprintf(path, "%s/logged_district", dist);

    struct stat st;
    if (stat(path, &st) < 0) return;

    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) return;

    time_t now = time(NULL);
    dprintf(fd, "[%ld] %s (%s): %s\n", now, user, role, msg);
    close(fd);
}

int parse_condition(const char *input, char *field, char *op, char *value) {
    return sscanf(input, "%[^:]:%[^:]:%s", field, op, value) == 3;
}

int match_condition(Report *r, const char *field, const char *op, const char *value) {
    if (!strcmp(field, "severity")) {
        int v = atoi(value);
        if (!strcmp(op,"==")) return r->severity == v;
        if (!strcmp(op,">=")) return r->severity >= v;
        if (!strcmp(op,"<=")) return r->severity <= v;
        if (!strcmp(op,">")) return r->severity > v;
        if (!strcmp(op,"<")) return r->severity < v;
        if (!strcmp(op,"!=")) return r->severity != v;
    }
    if (!strcmp(field, "category")) {
        if (!strcmp(op,"==")) return strcmp(r->category,value)==0;
        if (!strcmp(op,"!=")) return strcmp(r->category,value)!=0;
    }
    if (!strcmp(field, "inspector")) {
        if (!strcmp(op,"==")) return strcmp(r->inspector,value)==0;
    }
    if (!strcmp(field, "timestamp")) {
        time_t v = atol(value);
        if (!strcmp(op,"==")) return r->timestamp == v;
        if (!strcmp(op,">=")) return r->timestamp >= v;
        if (!strcmp(op,"<=")) return r->timestamp <= v;
        if (!strcmp(op,">")) return r->timestamp > v;
        if (!strcmp(op,"<")) return r->timestamp < v;
    }
    return 0;
}

void ensure_files(const char *dist) {
    mkdir(dist, 0750);
    chmod(dist, 0750);

    char path[256];
    int fd;

    sprintf(path, "%s/reports.dat", dist);
    fd = open(path, O_CREAT, 0664);
    if (fd >= 0) { close(fd); chmod(path, 0664); }

    sprintf(path, "%s/district.cfg", dist);
    fd = open(path, O_CREAT, 0640);
    if (fd >= 0) { close(fd); chmod(path, 0640); }

    sprintf(path, "%s/logged_district", dist);
    fd = open(path, O_CREAT, 0644);
    if (fd >= 0) { close(fd); chmod(path, 0644); }
}

void add_report(const char *dist, const char *user, const char *role) {
    ensure_files(dist);

    char path[256];
    sprintf(path, "%s/reports.dat", dist);

    struct stat st;
    if (stat(path, &st) < 0) return;

    if (!check_permission(&st, role, S_IWUSR)) {
        printf("Access denied\n");
        return;
    }

    int fd = open(path, O_WRONLY | O_APPEND);
    if (fd < 0) return;

    Report r;
    r.report_id = rand() % 10000;
    strncpy(r.inspector, user, 31);
    r.inspector[31] = 0;
    r.timestamp = time(NULL);

    scanf("%s", r.category);
    scanf("%d", &r.severity);
    scanf("%lf %lf", &r.lat, &r.lon);
    scanf(" %[^\n]", r.description);

    write(fd, &r, sizeof(Report));
    
    FILE *mf = fopen(".monitor_pid", "r");
    int m_pid;
    if (mf && fscanf(mf, "%d", &m_pid) == 1) {
        if (kill(m_pid, SIGUSR1) == 0) {
            log_action(dist, user, role, "ADD - Monitor notificat cu succes");
        } else {
            log_action(dist, user, role, "ADD - Eroare la trimitere semnal");
        }
        fclose(mf);
    } else {
        log_action(dist, user, role, "ADD - Monitorul nu a putut fi informat (PID inexistent)");
    }

    close(fd);
    log_action(dist, user, role, "ADD");
}

void list_reports(const char *dist, const char *role) {
    char path[256];
    sprintf(path, "%s/reports.dat", dist);

    struct stat st;
    if (stat(path, &st) < 0) return;

    if (!check_permission(&st, role, S_IRUSR)) {
        printf("Access denied\n");
        return;
    }

    char perms[11];
    get_permissions_string(st.st_mode, perms);
    printf("Perms: %s Size: %ld Mod: %s", perms, st.st_size, ctime(&st.st_mtime));

    int fd = open(path, O_RDONLY);
    if (fd < 0) return;

    Report r;
    while (read(fd, &r, sizeof(Report)) > 0) {
        printf("ID:%d %s S:%d\n", r.report_id, r.category, r.severity);
    }
    close(fd);
}

void view_report(const char *dist, int id, const char *role) {
    char path[256];
    sprintf(path, "%s/reports.dat", dist);

    struct stat st;
    if (stat(path, &st) < 0) return;

    if (!check_permission(&st, role, S_IRUSR)) {
        printf("Access denied\n");
        return;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) return;

    Report r;
    while (read(fd, &r, sizeof(Report)) > 0) {
        if (r.report_id == id) {
            printf("ID:%d\nInspector:%s\nCategory:%s\nSeverity:%d\n",
                   r.report_id, r.inspector, r.category, r.severity);
        }
    }
    close(fd);
}

void remove_report(const char *dist, int id, const char *role, const char *user) {
    if (strcmp(role, "manager") != 0) {
        printf("Access denied\n");
        return;
    }

    char path[256];
    sprintf(path, "%s/reports.dat", dist);

    int fd = open(path, O_RDWR);
    if (fd < 0) return;

    Report r;
    off_t pos = 0;
    int found = 0;

    while (read(fd, &r, sizeof(Report)) > 0) {
        if (r.report_id == id) { found = 1; break; }
        pos += sizeof(Report);
    }

    if (!found) { close(fd); return; }

    off_t next_pos = pos + sizeof(Report);
    Report next;

    while (1) {
        lseek(fd, next_pos, SEEK_SET);
        if (read(fd, &next, sizeof(Report)) <= 0) break;
        lseek(fd, pos, SEEK_SET);
        write(fd, &next, sizeof(Report));
        pos += sizeof(Report);
        next_pos += sizeof(Report);
    }

    ftruncate(fd, pos);
    close(fd);

    log_action(dist, user, role, "REMOVE");
}

void update_threshold(const char *dist, int val, const char *role) {
    if (strcmp(role, "manager") != 0) {
        printf("Access denied\n");
        return;
    }

    char path[256];
    sprintf(path, "%s/district.cfg", dist);

    struct stat st;
    if (stat(path, &st) < 0) return;

    if ((st.st_mode & 0777) != 0640) {
        printf("Wrong permissions\n");
        return;
    }

    FILE *f = fopen(path, "w");
    if (!f) return;

    fprintf(f, "%d", val);
    fclose(f);
    chmod(path, 0640);
}

void filter_reports(const char *dist, int argc, char *argv[], const char *role) {
    char path[256];
    sprintf(path, "%s/reports.dat", dist);

    struct stat st;
    if (stat(path, &st) < 0) return;

    if (!check_permission(&st, role, S_IRUSR)) {
        printf("Access denied\n");
        return;
    }

    int fd = open(path, O_RDONLY);
    if (fd < 0) return;

    Report r;
    while (read(fd, &r, sizeof(Report)) > 0) {
        int ok = 1;
        for (int i = 0; i < argc; i++) {
            if (strchr(argv[i], ':') == NULL) continue;
            char f[32], o[5], v[64];
            if (parse_condition(argv[i], f, o, v)) {
                if (!match_condition(&r, f, o, v)) {
                    ok = 0;
                    break;
                }
            }
        }
        if (ok) printf("ID:%d\n", r.report_id);
    }
    close(fd);
}

void remove_district(const char *dist, const char *role) {
    if (strcmp(role, "manager") != 0) {
        printf("Acces refuzat: Doar managerul poate sterge districte.\n");
        return;
    }
    pid_t pid = fork();
    if (pid == 0) {
        execlp("rm", "rm", "-rf", dist, (char *)NULL);
        exit(1);
    } else {
        wait(NULL);
        char lnk[128];
        sprintf(lnk, "active_reports-%s", dist);
        unlink(lnk);
        printf("Districtul %s si symlink-ul sau au fost sterse.\n", dist);
    }
}

int main(int argc, char *argv[]) {
    srand(time(NULL));  
    char *role = "inspector";
    char *user = "user";
    char *cmd = NULL;
    char *dist = NULL;
    int val = -1;
    int filter_start_idx = -1;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--role")) role = argv[++i];
        else if (!strcmp(argv[i], "--user")) user = argv[++i];
        else if (!cmd) {
            cmd = argv[i];
            if (strcmp(cmd, "filter") == 0) filter_start_idx = i + 2;
        }
        else if (!dist) dist = argv[i];
        else if (val == -1) val = atoi(argv[i]);
    }

    if (!cmd || !dist) return 0;

    char lnk[128], tgt[128];
    sprintf(lnk, "active_reports-%s", dist);
    sprintf(tgt, "%s/reports.dat", dist);

    struct stat st;
    if (lstat(lnk, &st) == 0) {
        if (stat(lnk, &st) != 0)
            printf("Dangling link: %s\n", lnk);
    } else {
        symlink(tgt, lnk);
    }

    if (!strcmp(cmd, "add")) add_report(dist, user, role);
    else if (!strcmp(cmd, "list")) list_reports(dist, role);
    else if (!strcmp(cmd, "view")) view_report(dist, val, role);
    else if (!strcmp(cmd, "remove_report")) remove_report(dist, val, role, user);
    else if (!strcmp(cmd, "update_threshold")) update_threshold(dist, val, role);
    else if (!strcmp(cmd, "remove_district")) remove_district(dist, role);
    else if (!strcmp(cmd, "filter")) {
        if (filter_start_idx != -1 && filter_start_idx < argc) {
            filter_reports(dist, argc - filter_start_idx, &argv[filter_start_idx], role);
        }
    }

    return 0;
}
