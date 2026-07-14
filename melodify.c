#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winmm.lib")

#define MAX_SONGS 100
#define BUFFER_SIZE 32768

char currentSongAlias[50] = "";
char currentSongName[100] = "";
int isPlaying = 0;

// Playlist
char playlist[MAX_SONGS][100] = {
    "NUMB.mp3", "Criminal.mp3", "Fever.mp3", "IDGAF.mp3", "Jhol.mp3", "Maniac.mp3",
    "Physical.mp3", "Escapism..mp3", "Espresso.mp3", "Judas.mp3", "Levitating.mp3",
    "Monster.mp3", "older.mp3", "Paparazzi.mp3", "Paro.mp3", "Reflections.mp3",
    "Softcore.mp3", "Sprinter.mp3", "Strangers.mp3", "Thumbs.mp3", "Unforgettable.mp3", "YAD.mp3", 
    "Suffocation.mp3", "AURA.mp3", "Bilionera.mp3", "Cherry.mp3", "Confident.mp3", "Empathy.mp3",
    "Havana.mp3", "Sucker.mp3"
};

int playlistSize = 30;
char userPlaylist[MAX_SONGS][100];
int userPlaylistSize = 0;
int currentIndex = 0;
int lastPlayed = -1;

// Function declarations
void stopCurrentSong();
void playSong(const char* song);
void sendHttpResponse(SOCKET clientSocket, const char* contentType, const char* content);
void handleRequest(SOCKET clientSocket, char* request);
void serveFile(SOCKET clientSocket, const char* path);
void generatePlaylistJSON(SOCKET clientSocket);
void generateUserPlaylistJSON(SOCKET clientSocket);
void generateMoodJSON(SOCKET clientSocket, const char* mood);
void generateGenreJSON(SOCKET clientSocket, const char* genre);

// Stop current song
void stopCurrentSong() {
    if (strlen(currentSongAlias) > 0) {
        char command[100];
        sprintf(command, "close %s", currentSongAlias);
        mciSendString(command, NULL, 0, NULL);
        currentSongAlias[0] = '\0';
        isPlaying = 0;
    }
}

// Play a song
void playSong(const char* song) {
    stopCurrentSong();
    
    char fullPath[256];
    sprintf(fullPath, "%s", song);
    
    char command[500];
    sprintf(currentSongAlias, "song%ld", time(NULL));
    sprintf(command, "open \"%s\" type mpegvideo alias %s", fullPath, currentSongAlias);
    
    if (mciSendString(command, NULL, 0, NULL) != 0) {
        printf("Error playing song: %s\n", song);
        return;
    }
    
    sprintf(command, "play %s", currentSongAlias);
    mciSendString(command, NULL, 0, NULL);
    
    strcpy(currentSongName, song);
    isPlaying = 1;
    printf("Now playing: %s\n", song);
}

// Pause current song
void pauseSong() {
    if (strlen(currentSongAlias) > 0 && isPlaying) {
        char command[100];
        sprintf(command, "pause %s", currentSongAlias);
        mciSendString(command, NULL, 0, NULL);
        isPlaying = 0;
        printf("Paused: %s\n", currentSongName);
    }
}

// Resume current song
void resumeSong() {
    if (strlen(currentSongAlias) > 0 && !isPlaying) {
        char command[100];
        sprintf(command, "resume %s", currentSongAlias);
        mciSendString(command, NULL, 0, NULL);
        isPlaying = 1;
        printf("Resumed: %s\n", currentSongName);
    }
}

// Get current position and duration
void getSongProgress(char* response) {
    if (strlen(currentSongAlias) == 0) {
        sprintf(response, "{\"progress\":0,\"duration\":100,\"percentage\":0}");
        return;
    }
    
    char position[50], duration[50];
    char command[100];
    
    sprintf(command, "status %s position", currentSongAlias);
    mciSendString(command, position, sizeof(position), NULL);
    
    sprintf(command, "status %s length", currentSongAlias);
    mciSendString(command, duration, sizeof(duration), NULL);
    
    int pos = atoi(position);
    int dur = atoi(duration);
    int percentage = (dur > 0) ? (pos * 100 / dur) : 0;
    
    sprintf(response, "{\"progress\":%d,\"duration\":%d,\"percentage\":%d}", pos, dur, percentage);
}

// Next song from main playlist
void nextSong() {
    if (playlistSize > 0) {
        currentIndex = (currentIndex + 1) % playlistSize;
        playSong(playlist[currentIndex]);
        lastPlayed = currentIndex;
    }
}

// Previous song from main playlist
void previousSong() {
    if (playlistSize > 0) {
        if (lastPlayed == -1) {
            currentIndex = (currentIndex - 1 + playlistSize) % playlistSize;
        } else {
            currentIndex = lastPlayed;
        }
        playSong(playlist[currentIndex]);
        lastPlayed = currentIndex;
    }
}

// Send HTTP response
void sendHttpResponse(SOCKET clientSocket, const char* contentType, const char* content) {
    char response[BUFFER_SIZE];
    sprintf(response, 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s",
        contentType, (int)strlen(content), content);
    send(clientSocket, response, strlen(response), 0);
}

// Serve HTML/CSS/JS files
void serveFile(SOCKET clientSocket, const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        sendHttpResponse(clientSocket, "text/plain", "404 Not Found");
        return;
    }
    
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = (char*)malloc(fileSize + 1);
    fread(content, 1, fileSize, file);
    content[fileSize] = '\0';
    fclose(file);
    
    const char* contentType = "text/html";
    if (strstr(path, ".css")) contentType = "text/css";
    else if (strstr(path, ".js")) contentType = "application/javascript";
    
    sendHttpResponse(clientSocket, contentType, content);
    free(content);
}

// Generate main playlist JSON
void generatePlaylistJSON(SOCKET clientSocket) {
    char json[BUFFER_SIZE] = "{\"songs\":[";
    
    for (int i = 0; i < playlistSize; i++) {
        char songName[100];
        strcpy(songName, playlist[i]);
        char* dot = strstr(songName, ".mp3");
        if (dot) *dot = '\0';
        
        char entry[200];
        sprintf(entry, "{\"id\":%d,\"name\":\"%s\",\"file\":\"%s\"}", 
                i, songName, playlist[i]);
        strcat(json, entry);
        if (i < playlistSize - 1) strcat(json, ",");
    }
    
    strcat(json, "]}");
    sendHttpResponse(clientSocket, "application/json", json);
}

// Generate user playlist JSON
void generateUserPlaylistJSON(SOCKET clientSocket) {
    char json[BUFFER_SIZE] = "{\"songs\":[";
    
    for (int i = 0; i < userPlaylistSize; i++) {
        char songName[100];
        strcpy(songName, userPlaylist[i]);
        char* dot = strstr(songName, ".mp3");
        if (dot) *dot = '\0';
        
        char entry[200];
        sprintf(entry, "{\"id\":%d,\"name\":\"%s\",\"file\":\"%s\"}", 
                i, songName, userPlaylist[i]);
        strcat(json, entry);
        if (i < userPlaylistSize - 1) strcat(json, ",");
    }
    
    strcat(json, "]}");
    sendHttpResponse(clientSocket, "application/json", json);
}

// Add song to user playlist
void addToUserPlaylist(const char* songFile) {
    if (userPlaylistSize < MAX_SONGS) {
        strcpy(userPlaylist[userPlaylistSize], songFile);
        userPlaylistSize++;
        printf("Added to user playlist: %s\n", songFile);
    }
}

// Generate mood-based recommendations JSON
void generateMoodJSON(SOCKET clientSocket, const char* mood) {
    char json[BUFFER_SIZE] = "{\"songs\":[";
    int first = 1;
    
    for (int i = 0; i < playlistSize; i++) {
        int match = 0;
        
        if (strcmp(mood, "happy") == 0 && (strstr(playlist[i], "IDGAF") || strstr(playlist[i], "Physical") || 
            strstr(playlist[i], "Confident") || strstr(playlist[i], "Levitating") || strstr(playlist[i], "Judas") ||
            strstr(playlist[i], "Fever") || strstr(playlist[i], "Bilionera") || strstr(playlist[i], "Paro") ||
            strstr(playlist[i], "Sucker") || strstr(playlist[i], "Sprinter") || strstr(playlist[i], "AURA"))) {
            match = 1;
        }
        else if (strcmp(mood, "energetic") == 0 && (strstr(playlist[i], "AURA") || strstr(playlist[i], "Sucker") || 
                 strstr(playlist[i], "Bilionera") || strstr(playlist[i], "Judas") || strstr(playlist[i], "Confident") ||
                 strstr(playlist[i], "IDGAF"))) {
            match = 1;
        }
        else if (strcmp(mood, "emotional") == 0 && (strstr(playlist[i], "NUMB") || strstr(playlist[i], "older") || 
                 strstr(playlist[i], "Escapism") || strstr(playlist[i], "Softcore") || strstr(playlist[i], "Reflections") ||
                 strstr(playlist[i], "Empathy") || strstr(playlist[i], "Suffocation") || strstr(playlist[i], "YAD") ||
                 strstr(playlist[i], "Maniac") || strstr(playlist[i], "Strangers") || strstr(playlist[i], "Monster") ||
                 strstr(playlist[i], "Cherry"))) {
            match = 1;
        }
        else if (strcmp(mood, "lonely") == 0 && (strstr(playlist[i], "NUMB") || strstr(playlist[i], "older") || 
                 strstr(playlist[i], "Escapism") || strstr(playlist[i], "Unforgettable") || strstr(playlist[i], "Softcore") ||
                 strstr(playlist[i], "Thumbs") || strstr(playlist[i], "Empathy") || strstr(playlist[i], "Cherry") ||
                 strstr(playlist[i], "Suffocation"))) {
            match = 1;
        }
        else if (strcmp(mood, "party") == 0 && (strstr(playlist[i], "Fever") || strstr(playlist[i], "Levitating") || 
                 strstr(playlist[i], "Bilionera") || strstr(playlist[i], "Espresso") || strstr(playlist[i], "Havana") ||
                 strstr(playlist[i], "Confident") || strstr(playlist[i], "Sucker") || strstr(playlist[i], "Physical") ||
                 strstr(playlist[i], "Paro") || strstr(playlist[i], "Sprinter") || strstr(playlist[i], "AURA"))) {
            match = 1;
        }
        else if (strcmp(mood, "chill") == 0 && (strstr(playlist[i], "Reflections") || strstr(playlist[i], "Softcore") || 
                 strstr(playlist[i], "Cherry") || strstr(playlist[i], "older") || strstr(playlist[i], "YAD") ||
                 strstr(playlist[i], "Strangers") || strstr(playlist[i], "Unforgettable"))) {
            match = 1;
        }
        else if (strcmp(mood, "dreamy") == 0 && (strstr(playlist[i], "Unforgettable") || strstr(playlist[i], "YAD") || 
                 strstr(playlist[i], "Cherry") || strstr(playlist[i], "Softcore") || strstr(playlist[i], "Reflections") ||
                 strstr(playlist[i], "older") || strstr(playlist[i], "Strangers"))) {
            match = 1;
        }
        else if (strcmp(mood, "rebellious") == 0 && (strstr(playlist[i], "Judas") || strstr(playlist[i], "Maniac") || 
                 strstr(playlist[i], "NUMB") || strstr(playlist[i], "Escapism") || strstr(playlist[i], "Monster") ||
                 strstr(playlist[i], "Empathy") || strstr(playlist[i], "Suffocation") || strstr(playlist[i], "AURA"))) {
            match = 1;
        }
        else if (strcmp(mood, "groovy") == 0 && (strstr(playlist[i], "Espresso") || strstr(playlist[i], "Havana") || 
                 strstr(playlist[i], "Unforgettable") || strstr(playlist[i], "Fever") || strstr(playlist[i], "Cherry") ||
                 strstr(playlist[i], "Paro"))) {
            match = 1;
        }
        
        if (match) {
            char songName[100];
            strcpy(songName, playlist[i]);
            char* dot = strstr(songName, ".mp3");
            if (dot) *dot = '\0';
            
            char entry[200];
            sprintf(entry, "%s{\"name\":\"%s\",\"file\":\"%s\"}", 
                    first ? "" : ",", songName, playlist[i]);
            strcat(json, entry);
            first = 0;
        }
    }
    
    strcat(json, "]}");
    sendHttpResponse(clientSocket, "application/json", json);
}

// Generate genre-based recommendations JSON
void generateGenreJSON(SOCKET clientSocket, const char* genre) {
    char json[BUFFER_SIZE] = "{\"songs\":[";
    int first = 1;
    
    for (int i = 0; i < playlistSize; i++) {
        int match = 0;
        
        if (strcmp(genre, "pop") == 0 && (strstr(playlist[i], "Criminal") || strstr(playlist[i], "Fever") || 
            strstr(playlist[i], "IDGAF") || strstr(playlist[i], "Physical") || strstr(playlist[i], "Espresso") ||
            strstr(playlist[i], "Paparazzi") || strstr(playlist[i], "Judas") || strstr(playlist[i], "Confident") ||
            strstr(playlist[i], "Havana") || strstr(playlist[i], "Sucker") || strstr(playlist[i], "Thumbs") ||
            strstr(playlist[i], "Strangers") || strstr(playlist[i], "Cherry"))) {
            match = 1;
        }
        else if (strcmp(genre, "indie") == 0 && (strstr(playlist[i], "NUMB") || strstr(playlist[i], "Escapism") || 
                 strstr(playlist[i], "Monster") || strstr(playlist[i], "Maniac") || strstr(playlist[i], "older") ||
                 strstr(playlist[i], "Reflections") || strstr(playlist[i], "Softcore") || strstr(playlist[i], "YAD") ||
                 strstr(playlist[i], "Cherry"))) {
            match = 1;
        }
        else if (strcmp(genre, "electronic") == 0 && (strstr(playlist[i], "Suffocation") || strstr(playlist[i], "Empathy") || 
                 strstr(playlist[i], "AURA") || strstr(playlist[i], "Bilionera"))) {
            match = 1;
        }
        else if (strcmp(genre, "hiphop") == 0 && (strstr(playlist[i], "Sprinter") || strstr(playlist[i], "Unforgettable") || 
                 strstr(playlist[i], "NUMB"))) {
            match = 1;
        }
        
        if (match) {
            char songName[100];
            strcpy(songName, playlist[i]);
            char* dot = strstr(songName, ".mp3");
            if (dot) *dot = '\0';
            
            char entry[200];
            sprintf(entry, "%s{\"name\":\"%s\",\"file\":\"%s\"}", 
                    first ? "" : ",", songName, playlist[i]);
            strcat(json, entry);
            first = 0;
        }
    }
    
    strcat(json, "]}");
    sendHttpResponse(clientSocket, "application/json", json);
}

// Search song
void searchSong(SOCKET clientSocket, const char* query) {
    char json[BUFFER_SIZE] = "{\"songs\":[";
    int first = 1;
    
    for (int i = 0; i < playlistSize; i++) {
        if (strstr(playlist[i], query) != NULL) {
            char songName[100];
            strcpy(songName, playlist[i]);
            char* dot = strstr(songName, ".mp3");
            if (dot) *dot = '\0';
            
            char entry[200];
            sprintf(entry, "%s{\"name\":\"%s\",\"file\":\"%s\"}", 
                    first ? "" : ",", songName, playlist[i]);
            strcat(json, entry);
            first = 0;
        }
    }
    
    strcat(json, "]}");
    sendHttpResponse(clientSocket, "application/json", json);
}

// Handle HTTP requests
void handleRequest(SOCKET clientSocket, char* request) {
    char method[10], path[256];
    sscanf(request, "%s %s", method, path);
    
    printf("Request: %s %s\n", method, path);
    
    // API endpoints
    if (strcmp(path, "/api/playlist") == 0) {
        generatePlaylistJSON(clientSocket);
    }
    else if (strcmp(path, "/api/userplaylist") == 0) {
        generateUserPlaylistJSON(clientSocket);
    }
    else if (strstr(path, "/api/play") != NULL) {
        char songFile[256];
        sscanf(path, "/api/play?file=%[^\n&]", songFile);
        // URL decode
        for (int i = 0; songFile[i]; i++) {
            if (songFile[i] == '%' && songFile[i+1] && songFile[i+2]) {
                if (songFile[i+1] == '2' && songFile[i+2] == '0') {
                    songFile[i] = ' ';
                    memmove(&songFile[i+1], &songFile[i+3], strlen(&songFile[i+3]) + 1);
                }
            }
        }
        playSong(songFile);
        sendHttpResponse(clientSocket, "application/json", "{\"status\":\"playing\"}");
    }
    else if (strstr(path, "/api/addtoplaylist") != NULL) {
        char songFile[256];
        sscanf(path, "/api/addtoplaylist?file=%[^\n&]", songFile);
        addToUserPlaylist(songFile);
        sendHttpResponse(clientSocket, "application/json", "{\"status\":\"added\"}");
    }
    else if (strcmp(path, "/api/pause") == 0) {
        pauseSong();
        sendHttpResponse(clientSocket, "application/json", "{\"status\":\"paused\"}");
    }
    else if (strcmp(path, "/api/resume") == 0) {
        resumeSong();
        sendHttpResponse(clientSocket, "application/json", "{\"status\":\"resumed\"}");
    }
    else if (strcmp(path, "/api/next") == 0) {
        nextSong();
        sendHttpResponse(clientSocket, "application/json", "{\"status\":\"next\"}");
    }
    else if (strcmp(path, "/api/previous") == 0) {
        previousSong();
        sendHttpResponse(clientSocket, "application/json", "{\"status\":\"previous\"}");
    }
    else if (strcmp(path, "/api/progress") == 0) {
        char progress[200];
        getSongProgress(progress);
        sendHttpResponse(clientSocket, "application/json", progress);
    }
    else if (strstr(path, "/api/mood") != NULL) {
        char mood[50];
        sscanf(path, "/api/mood?mood=%s", mood);
        generateMoodJSON(clientSocket, mood);
    }
    else if (strstr(path, "/api/genre") != NULL) {
        char genre[50];
        sscanf(path, "/api/genre?genre=%s", genre);
        generateGenreJSON(clientSocket, genre);
    }
    else if (strstr(path, "/api/search") != NULL) {
        char query[100];
        sscanf(path, "/api/search?q=%s", query);
        searchSong(clientSocket, query);
    }
    else if (strcmp(path, "/api/nowplaying") == 0) {
        char response[200];
        char songName[100];
        strcpy(songName, currentSongName);
        char* dot = strstr(songName, ".mp3");
        if (dot) *dot = '\0';
        sprintf(response, "{\"song\":\"%s\",\"isPlaying\":%d}", songName, isPlaying);
        sendHttpResponse(clientSocket, "application/json", response);
    }
    else if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        serveFile(clientSocket, "web/index.html");
    }
    else if (strstr(path, ".css")) {
        char filepath[256];
        sprintf(filepath, "web%s", path);
        serveFile(clientSocket, filepath);
    }
    else if (strstr(path, ".js")) {
        char filepath[256];
        sprintf(filepath, "web%s", path);
        serveFile(clientSocket, filepath);
    }
    else {
        sendHttpResponse(clientSocket, "text/plain", "404 Not Found");
    }
}

// Start web server
void startWebServer() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrLen = sizeof(clientAddr);
    char buffer[BUFFER_SIZE];
    
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("WSAStartup failed\n");
        return;
    }
    
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        printf("Socket creation failed\n");
        WSACleanup();
        return;
    }
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8080);
    
    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        printf("Bind failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return;
    }
    
    if (listen(serverSocket, 10) == SOCKET_ERROR) {
        printf("Listen failed\n");
        closesocket(serverSocket);
        WSACleanup();
        return;
    }
    
    printf("\n=========================================\n");
    printf("🎵 Melodify Server Started!\n");
    printf("📍 Open your browser and go to: http://localhost:8080\n");
    printf("🎶 Enjoy your Spotify-like experience!\n");
    printf("=========================================\n\n");
    
    while (1) {
        clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSocket == INVALID_SOCKET) {
            continue;
        }
        
        int bytesReceived = recv(clientSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            handleRequest(clientSocket, buffer);
        }
        
        closesocket(clientSocket);
    }
    
    closesocket(serverSocket);
    WSACleanup();
}

// Console menu function (kept for backward compatibility)
void printMenu() {
    system("cls");
    printf("\033[1;35mMELODIFY\n");
    printf("\033[0;34mWeb interface available at http://localhost:8080\033[0m\n");
    printf("\033[0;34mPress Ctrl+C to exit\033[0m\n");
}

int main() {
    srand(time(NULL));
    
    printf("\n🎵 Welcome to Melodify!\n");
    printf("Starting web server...\n");
    
    // Start web server in a separate thread
    startWebServer();
    
    return 0;
}