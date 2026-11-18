/**
 * ===================================================================
 *  PAPERLESS OFFICE & STUDENT PORTAL - C BACKEND & LOCAL DATABASE
 * ===================================================================
 *  Features:
 *   1. Persistent Binary File Database (records.db) + CSV Export (records.csv)
 *   2. Student & Document Records Management (Add, Delete, List, Search)
 *   3. Integrated Winsock HTTP Web Server & REST API (Port 8080)
 *   4. Multi-user Local Network Sharing for other students (0.0.0.0)
 *   5. Static file serving (index.html, style.css, script.js)
 * ===================================================================
 */

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#if defined(__clang__)
#ifndef _X86INTRIN_H_INCLUDED
#define _X86INTRIN_H_INCLUDED
#endif
#ifndef _X86GPRINTRIN_H_INCLUDED
#define _X86GPRINTRIN_H_INCLUDED
#endif
#ifndef _IMMINTRIN_H_INCLUDED
#define _IMMINTRIN_H_INCLUDED
#endif
#ifndef _IA32INTRIN_H_INCLUDED
#define _IA32INTRIN_H_INCLUDED
#endif
#endif

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#define MAX_RECORDS 2000
#define DB_FILE "records.db"
#define CSV_FILE "records.csv"
#define PORT 8080
#define BUFFER_SIZE 65536

// Document & Student Record Data Structure
typedef struct {
    int id;                   // Unique Record ID
    char student_name[64];    // Student / Author Name
    char student_id[32];      // Student Roll / Reg Number
    char doc_name[128];       // Document Title
    char category[48];        // Category (Assignment, Lab, Project, Notes, Office)
    char doc_note[512];       // Document Notes / Content
    char created_at[32];      // Timestamp (YYYY-MM-DD HH:MM:SS)
} DocumentRecord;

static DocumentRecord records[MAX_RECORDS];
static int record_count = 0;

// Default Seed Records for First-Time Run
static const DocumentRecord SEED_RECORDS[4] = {
    { 101, "Rahul Sharma", "R24CS101", "Data Structures Lab Report", "Lab Report", "Completed Binary Search Tree & AVL Tree implementations with test benchmarks.", "2026-11-18 10:15:00" },
    { 102, "Priya Nair", "R24EC204", "Digital Electronics Notes", "Notes", "Unit 3 Logic Gates and Karnaugh Map simplification cheat-sheet.", "2026-11-18 11:30:00" },
    { 103, "Alex Johnson", "R24CS155", "Cloud Computing Project Proposal", "Project", "Architecture diagram and cost breakdown for containerized microservices on AWS.", "2026-11-18 14:05:00" },
    { 104, "Ananya Verma", "R24AI088", "AI Ethics Seminar Paper", "Assignment", "Literature review on algorithmic fairness, bias mitigation, and data privacy in LLMs.", "2026-11-18 15:40:00" }
};

// Forward Declarations
void load_database(void);
void save_database(void);
void export_csv(void);
void reset_seed_database(void);
int add_record(const char *student_name, const char *student_id, const char *doc_name, const char *category, const char *doc_note);
int delete_record(int id);
void get_current_timestamp(char *buffer, size_t max_len);
void print_records_table(void);
void run_web_server(void);

// Get current timestamp formatted as YYYY-MM-DD HH:MM:SS
void get_current_timestamp(char *buffer, size_t max_len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    if (t) {
        strftime(buffer, max_len, "%Y-%m-%d %H:%M:%S", t);
    } else {
        snprintf(buffer, max_len, "2026-11-18 12:00:00");
    }
}

// Escape JSON strings
static void json_escape(const char *src, char *dest, size_t max_len) {
    size_t j = 0;
    for (size_t i = 0; src[i] != '\0' && j + 2 < max_len; i++) {
        if (src[i] == '"' || src[i] == '\\') {
            dest[j++] = '\\';
            dest[j++] = src[i];
        } else if (src[i] == '\n') {
            dest[j++] = '\\';
            dest[j++] = 'n';
        } else if (src[i] == '\r') {
            dest[j++] = '\\';
            dest[j++] = 'r';
        } else if (src[i] == '\t') {
            dest[j++] = '\\';
            dest[j++] = 't';
        } else {
            dest[j++] = src[i];
        }
    }
    dest[j] = '\0';
}

// Save Database to Binary File and export CSV
void save_database(void) {
    FILE *fp = fopen(DB_FILE, "wb");
    if (fp) {
        fwrite(&record_count, sizeof(int), 1, fp);
        if (record_count > 0) {
            fwrite(records, sizeof(DocumentRecord), record_count, fp);
        }
        fclose(fp);
    }
    export_csv();
}

// Export database records to human-readable CSV file
void export_csv(void) {
    FILE *fp = fopen(CSV_FILE, "w");
    if (!fp) return;

    fprintf(fp, "ID,Student Name,Student ID,Document Title,Category,Notes,Created At\n");
    for (int i = 0; i < record_count; i++) {
        fprintf(fp, "%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\"\n",
                records[i].id,
                records[i].student_name,
                records[i].student_id,
                records[i].doc_name,
                records[i].category,
                records[i].doc_note,
                records[i].created_at);
    }
    fclose(fp);
}

// Reset Database with default seed records
void reset_seed_database(void) {
    record_count = 4;
    for (int i = 0; i < 4; i++) {
        records[i] = SEED_RECORDS[i];
    }
    save_database();
    printf("[DATABASE] Initialized %s with 4 demo records.\n", DB_FILE);
}

// Load database from disk
void load_database(void) {
    FILE *fp = fopen(DB_FILE, "rb");
    if (!fp) {
        printf("[DATABASE] No existing database found. Seeding new database...\n");
        reset_seed_database();
        return;
    }

    int count = 0;
    if (fread(&count, sizeof(int), 1, fp) == 1) {
        if (count >= 0 && count <= MAX_RECORDS) {
            record_count = count;
            if (record_count > 0) {
                fread(records, sizeof(DocumentRecord), record_count, fp);
            }
        }
    }
    fclose(fp);

    if (record_count == 0) {
        reset_seed_database();
    } else {
        printf("[DATABASE] Successfully loaded %d records from %s.\n", record_count, DB_FILE);
    }
}

// Add a new document record
int add_record(const char *student_name, const char *student_id, const char *doc_name, const char *category, const char *doc_note) {
    if (record_count >= MAX_RECORDS) return -1;

    int new_id = 101;
    for (int i = 0; i < record_count; i++) {
        if (records[i].id >= new_id) {
            new_id = records[i].id + 1;
        }
    }

    DocumentRecord rec;
    memset(&rec, 0, sizeof(rec));
    rec.id = new_id;

    strncpy(rec.student_name, student_name && *student_name ? student_name : "Anonymous Student", sizeof(rec.student_name) - 1);
    strncpy(rec.student_id, student_id && *student_id ? student_id : "N/A", sizeof(rec.student_id) - 1);
    strncpy(rec.doc_name, doc_name && *doc_name ? doc_name : "Untitled Document", sizeof(rec.doc_name) - 1);
    strncpy(rec.category, category && *category ? category : "General", sizeof(rec.category) - 1);
    strncpy(rec.doc_note, doc_note && *doc_note ? doc_note : "(No notes)", sizeof(rec.doc_note) - 1);
    get_current_timestamp(rec.created_at, sizeof(rec.created_at));

    records[record_count++] = rec;
    save_database();
    return new_id;
}

// Delete record by ID
int delete_record(int id) {
    int found_index = -1;
    for (int i = 0; i < record_count; i++) {
        if (records[i].id == id) {
            found_index = i;
            break;
        }
    }

    if (found_index == -1) return 0;

    for (int i = found_index; i < record_count - 1; i++) {
        records[i] = records[i + 1];
    }
    record_count--;
    save_database();
    return 1;
}

// Serialize all records to JSON
static void serialize_to_json(char *buffer, size_t max_size) {
    size_t offset = 0;
    offset += snprintf(buffer + offset, max_size - offset, "[\n");

    for (int i = 0; i < record_count; i++) {
        char esc_sname[128], esc_sid[64], esc_doc[256], esc_cat[96], esc_note[1024];
        json_escape(records[i].student_name, esc_sname, sizeof(esc_sname));
        json_escape(records[i].student_id, esc_sid, sizeof(esc_sid));
        json_escape(records[i].doc_name, esc_doc, sizeof(esc_doc));
        json_escape(records[i].category, esc_cat, sizeof(esc_cat));
        json_escape(records[i].doc_note, esc_note, sizeof(esc_note));

        int written = snprintf(buffer + offset, max_size - offset,
            "  {\n"
            "    \"id\": %d,\n"
            "    \"student_name\": \"%s\",\n"
            "    \"student_id\": \"%s\",\n"
            "    \"doc_name\": \"%s\",\n"
            "    \"category\": \"%s\",\n"
            "    \"doc_note\": \"%s\",\n"
            "    \"created_at\": \"%s\"\n"
            "  }%s\n",
            records[i].id,
            esc_sname,
            esc_sid,
            esc_doc,
            esc_cat,
            esc_note,
            records[i].created_at,
            (i < record_count - 1) ? "," : "");

        if (written < 0 || (size_t)written >= max_size - offset) break;
        offset += written;
    }

    snprintf(buffer + offset, max_size - offset, "]\n");
}

// Simple JSON Key-Value Parser Helper
static void extract_json_value(const char *json, const char *key, char *out, size_t max_len) {
    out[0] = '\0';
    char search_pattern[64];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\"", key);

    const char *pos = strstr(json, search_pattern);
    if (!pos) return;

    pos += strlen(search_pattern);
    while (*pos == ' ' || *pos == ':' || *pos == '\t' || *pos == '\r' || *pos == '\n') pos++;

    if (*pos == '"') {
        pos++;
        size_t idx = 0;
        while (*pos && *pos != '"' && idx + 1 < max_len) {
            if (*pos == '\\' && *(pos + 1)) {
                pos++;
                if (*pos == 'n') out[idx++] = '\n';
                else if (*pos == 'r') out[idx++] = '\r';
                else if (*pos == 't') out[idx++] = '\t';
                else out[idx++] = *pos;
                pos++;
            } else {
                out[idx++] = *pos++;
            }
        }
        out[idx] = '\0';
    } else {
        size_t idx = 0;
        while (*pos && *pos != ',' && *pos != '}' && *pos != ' ' && *pos != '\r' && *pos != '\n' && idx + 1 < max_len) {
            out[idx++] = *pos++;
        }
        out[idx] = '\0';
    }
}

// Print all records as an ASCII table in CLI
void print_records_table(void) {
    printf("\n+-----+----------------------+-----------+--------------------------------+-----------------+---------------------+\n");
    printf("| %-3s | %-20s | %-9s | %-30s | %-15s | %-19s |\n", "ID", "Student Name", "Roll No", "Document Title", "Category", "Created At");
    printf("+-----+----------------------+-----------+--------------------------------+-----------------+---------------------+\n");

    if (record_count == 0) {
        printf("| %-107s |\n", "No documents found in local database.");
    } else {
        for (int i = 0; i < record_count; i++) {
            printf("| %-3d | %-20.20s | %-9.9s | %-30.30s | %-15.15s | %-19.19s |\n",
                   records[i].id,
                   records[i].student_name,
                   records[i].student_id,
                   records[i].doc_name,
                   records[i].category,
                   records[i].created_at);
        }
    }
    printf("+-----+----------------------+-----------+--------------------------------+-----------------+---------------------+\n");
    printf("Total Documents Stored: %d\n\n", record_count);
}

// Send HTTP Response
static void send_http_response(SOCKET client, int status_code, const char *status_text, const char *content_type, const char *body, size_t body_len) {
    char header[1024];
    int header_len = snprintf(header, sizeof(header),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Access-Control-Allow-Methods: GET, POST, DELETE, OPTIONS\r\n"
        "Access-Control-Allow-Headers: Content-Type, Accept\r\n"
        "Connection: close\r\n\r\n",
        status_code, status_text, content_type, body_len);

    send(client, header, header_len, 0);
    if (body_len > 0 && body) {
        send(client, body, (int)body_len, 0);
    }
}

// Serve a static file from disk
static void serve_static_file(SOCKET client, const char *filename, const char *content_type) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        const char *not_found = "{\"error\": \"404 File Not Found\"}";
        send_http_response(client, 404, "Not Found", "application/json", not_found, strlen(not_found));
        return;
    }

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = (char *)malloc(fsize + 1);
    if (!buf) {
        fclose(fp);
        const char *err = "{\"error\": \"500 Memory Error\"}";
        send_http_response(client, 500, "Internal Server Error", "application/json", err, strlen(err));
        return;
    }

    fread(buf, 1, fsize, fp);
    fclose(fp);

    send_http_response(client, 200, "OK", content_type, buf, fsize);
    free(buf);
}

// Print Server Banner and Local Network Info
void print_banner(void) {
    printf("====================================================================\n");
    printf("   PAPERLESS OFFICE & STUDENT PORTAL - C BACKEND SERVER\n");
    printf("====================================================================\n");
    printf("   Status: ACTIVE & LISTENING\n");
    printf("   Database: %s (%d records loaded)\n", DB_FILE, record_count);
    printf("   CSV Sync: %s (auto-updated)\n", CSV_FILE);
    printf("   ---------------------------------------------------------------\n");
    printf("   LOCAL ACCESS (Your Computer):\n");
    printf("     -> Web App:   http://localhost:%d\n", PORT);
    printf("     -> REST API:  http://localhost:%d/api/documents\n", PORT);
    printf("   ---------------------------------------------------------------\n");
    printf("   ANY OTHER STUDENTS (Classmates on same Wi-Fi / LAN):\n");
    printf("     -> Web App:   http://10.213.0.200:%d\n", PORT);
    printf("     -> Note: Any student can open this link on laptop or phone!\n");
    printf("   ---------------------------------------------------------------\n");
    printf("   API Endpoints:\n");
    printf("     [GET]    /api/documents    - List all stored student documents\n");
    printf("     [POST]   /api/documents    - Save a new student document\n");
    printf("     [POST]   /api/delete       - Delete document by ID { \"id\": 101 }\n");
    printf("     [GET]    /api/stats        - Summary statistics (total, students)\n");
    printf("     [POST]   /api/reset        - Reset to default demo documents\n");
    printf("   Press Ctrl+C in this terminal to stop the server.\n");
    printf("====================================================================\n\n");
}

// Web Server loop
void run_web_server(void) {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_addr_len = sizeof(client_addr);

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("[ERROR] WSAStartup failed. Error code: %d\n", WSAGetLastError());
        return;
    }

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == INVALID_SOCKET) {
        printf("[ERROR] Socket creation failed. Error code: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Bind to 0.0.0.0 so ANY student on Wi-Fi can connect
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("[ERROR] Bind failed on port %d. Error code: %d\n", PORT, WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return;
    }

    if (listen(server_socket, 20) == SOCKET_ERROR) {
        printf("[ERROR] Listen failed. Error code: %d\n", WSAGetLastError());
        closesocket(server_socket);
        WSACleanup();
        return;
    }

    print_banner();

    char *recv_buffer = (char *)malloc(BUFFER_SIZE);
    char *resp_buffer = (char *)malloc(BUFFER_SIZE * 2);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_socket == INVALID_SOCKET) continue;

        int bytes_received = recv(client_socket, recv_buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            recv_buffer[bytes_received] = '\0';

            char method[16] = {0}, url[256] = {0};
            sscanf(recv_buffer, "%15s %255s", method, url);

            char *client_ip = inet_ntoa(client_addr.sin_addr);

            printf("[HTTP] %s %-22s from %s\n", method, url, client_ip ? client_ip : "client");

            // Handle CORS OPTIONS Preflight
            if (strcmp(method, "OPTIONS") == 0) {
                send_http_response(client_socket, 204, "No Content", "text/plain", "", 0);
            }
            // Serve Web Application Frontend
            else if (strcmp(method, "GET") == 0 && (strcmp(url, "/") == 0 || strcmp(url, "/index.html") == 0)) {
                serve_static_file(client_socket, "index.html", "text/html; charset=utf-8");
            }
            else if (strcmp(method, "GET") == 0 && strcmp(url, "/style.css") == 0) {
                serve_static_file(client_socket, "style.css", "text/css; charset=utf-8");
            }
            else if (strcmp(method, "GET") == 0 && strcmp(url, "/script.js") == 0) {
                serve_static_file(client_socket, "script.js", "application/javascript; charset=utf-8");
            }
            else if (strcmp(method, "GET") == 0 && strcmp(url, "/favicon.ico") == 0) {
                send_http_response(client_socket, 204, "No Content", "image/x-icon", "", 0);
            }
            // API: GET Documents
            else if (strcmp(method, "GET") == 0 && strncmp(url, "/api/documents", 14) == 0) {
                serialize_to_json(resp_buffer, BUFFER_SIZE * 2);
                send_http_response(client_socket, 200, "OK", "application/json; charset=utf-8", resp_buffer, strlen(resp_buffer));
            }
            // API: GET Stats
            else if (strcmp(method, "GET") == 0 && strncmp(url, "/api/stats", 10) == 0) {
                // Count unique students
                char unique_students[MAX_RECORDS][64];
                int student_uniq_count = 0;
                for (int i = 0; i < record_count; i++) {
                    int already = 0;
                    for (int j = 0; j < student_uniq_count; j++) {
                        if (strcmp(unique_students[j], records[i].student_name) == 0) {
                            already = 1;
                            break;
                        }
                    }
                    if (!already && student_uniq_count < MAX_RECORDS) {
                        strncpy(unique_students[student_uniq_count++], records[i].student_name, 63);
                    }
                }

                snprintf(resp_buffer, BUFFER_SIZE,
                    "{\n"
                    "  \"status\": \"online\",\n"
                    "  \"total_documents\": %d,\n"
                    "  \"unique_students\": %d,\n"
                    "  \"database_file\": \"%s\",\n"
                    "  \"csv_file\": \"%s\",\n"
                    "  \"network_host\": \"10.213.0.200\",\n"
                    "  \"port\": %d\n"
                    "}\n",
                    record_count, student_uniq_count, DB_FILE, CSV_FILE, PORT);
                send_http_response(client_socket, 200, "OK", "application/json; charset=utf-8", resp_buffer, strlen(resp_buffer));
            }
            // API: POST Document (Add new record)
            else if (strcmp(method, "POST") == 0 && strncmp(url, "/api/documents", 14) == 0) {
                char *body = strstr(recv_buffer, "\r\n\r\n");
                if (body) body += 4;
                else body = "";

                char sname[64] = {0}, sid[32] = {0}, dname[128] = {0}, cat[48] = {0}, note[512] = {0};
                extract_json_value(body, "student_name", sname, sizeof(sname));
                extract_json_value(body, "student_id", sid, sizeof(sid));
                extract_json_value(body, "name", dname, sizeof(dname)); // also accept 'name' from previous format
                if (dname[0] == '\0') {
                    extract_json_value(body, "doc_name", dname, sizeof(dname));
                }
                extract_json_value(body, "category", cat, sizeof(cat));
                extract_json_value(body, "note", note, sizeof(note));
                if (note[0] == '\0') {
                    extract_json_value(body, "doc_note", note, sizeof(note));
                }

                int new_id = add_record(sname, sid, dname, cat, note);
                if (new_id > 0) {
                    snprintf(resp_buffer, BUFFER_SIZE, "{\"success\": true, \"id\": %d, \"message\": \"Document saved successfully\"}\n", new_id);
                    send_http_response(client_socket, 201, "Created", "application/json; charset=utf-8", resp_buffer, strlen(resp_buffer));
                    printf("  -> Added record ID #%d for student '%s' (\"%s\")\n", new_id, sname, dname);
                } else {
                    const char *err = "{\"success\": false, \"error\": \"Database is full\"}\n";
                    send_http_response(client_socket, 507, "Insufficient Storage", "application/json; charset=utf-8", err, strlen(err));
                }
            }
            // API: POST /api/delete or DELETE /api/documents
            else if ((strcmp(method, "POST") == 0 && strncmp(url, "/api/delete", 11) == 0) ||
                     (strcmp(method, "DELETE") == 0 && strncmp(url, "/api/documents", 14) == 0)) {
                int target_id = -1;
                char *q = strstr(url, "?id=");
                if (q) {
                    target_id = atoi(q + 4);
                } else {
                    char *body = strstr(recv_buffer, "\r\n\r\n");
                    if (body) {
                        body += 4;
                        char id_str[32] = {0};
                        extract_json_value(body, "id", id_str, sizeof(id_str));
                        target_id = atoi(id_str);
                    }
                }

                if (target_id > 0 && delete_record(target_id)) {
                    snprintf(resp_buffer, BUFFER_SIZE, "{\"success\": true, \"id\": %d, \"message\": \"Document deleted successfully\"}\n", target_id);
                    send_http_response(client_socket, 200, "OK", "application/json; charset=utf-8", resp_buffer, strlen(resp_buffer));
                    printf("  -> Deleted record ID #%d\n", target_id);
                } else {
                    const char *err = "{\"success\": false, \"error\": \"Record ID not found\"}\n";
                    send_http_response(client_socket, 404, "Not Found", "application/json; charset=utf-8", err, strlen(err));
                }
            }
            // API: POST /api/reset (Reset sample seed data)
            else if (strcmp(method, "POST") == 0 && strncmp(url, "/api/reset", 10) == 0) {
                reset_seed_database();
                const char *resp = "{\"success\": true, \"message\": \"Database reset to default seed records\"}\n";
                send_http_response(client_socket, 200, "OK", "application/json; charset=utf-8", resp, strlen(resp));
                printf("  -> Database reset to default seed records\n");
            }
            // 404 Not Found for any other route
            else {
                const char *not_found = "{\"error\": \"Route not found\"}\n";
                send_http_response(client_socket, 404, "Not Found", "application/json; charset=utf-8", not_found, strlen(not_found));
            }
        }
        closesocket(client_socket);
    }

    free(recv_buffer);
    free(resp_buffer);
    closesocket(server_socket);
    WSACleanup();
}

int main(int argc, char *argv[]) {
    // Set console code page to UTF-8 for clean output
    SetConsoleOutputCP(CP_UTF8);

    load_database();

    // Check if user requested CLI table only via argument
    if (argc > 1 && strcmp(argv[1], "--cli") == 0) {
        print_records_table();
        return 0;
    }

    run_web_server();
    return 0;
}
