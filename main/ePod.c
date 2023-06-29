#include <stdio.h>
#include <esp_log.h>
#include <freertos/freertos.h>
#include <freertos/task.h>
#include <string.h>
#include "jpeg.h"
#include "timeProbe.h"
#include "lcd.h"
#include "sd.h"
#include "taskMonitor.h"

uint16_t frameBuffer[240 * 240];

typedef uint32_t DWORD;
typedef uint32_t LONG;
typedef uint16_t WORD;
typedef char FOURCC;

struct RIFF {
    char magic[4];  /* "RIFF" */
    uint32_t fileSize;
    char fileType[4];
};

struct LIST {
    char magic[4];  /* "LIST"  */
    uint32_t listSize;
    char listType[4];
};

struct Chunk {
    char ckID[4];
    DWORD ckSize;
};


struct __attribute__((packed)) AVIFile {
    char magic[4];  /* "RIFF" */
    uint32_t fileSize;
    char fileType[4];
    struct {
        char magic[4];  /* "LIST" */
        uint32_t listSize;
        char listType[4];
        struct {
            char ckID[4];
            DWORD ckSize;
            struct {
                DWORD dwMicroSecPerFrame;
                DWORD dwMaxBytesPerSec;
                DWORD dwPaddingGranularity;
                DWORD dwFlags;
                DWORD dwTotalFrames;
                DWORD dwInitialFrames;
                DWORD dwStreams;
                DWORD dwSuggestedBufferSize;
                DWORD dwWidth;
                DWORD dwHeight;
                DWORD dwReserved[4];
            } MainAVIHeader;
        } AVIHeader;
        struct {
            char magic[4];  /* "LIST" */
            uint32_t listSize;
            char listType[4]; //strl
            struct {
                char ckID[4];
                DWORD ckSize;
                struct {
                    FOURCC fccType[4];
                    FOURCC fccHandler[4];
                    DWORD dwFlags;
                    WORD wPriority;
                    WORD wLanguage;
                    DWORD dwInitialFrames;
                    DWORD dwScale;
                    DWORD dwRate;
                    DWORD dwStart;
                    DWORD dwLength;
                    DWORD dwSuggestedBufferSize;
                    DWORD dwQuality;
                    DWORD dwSampleSize;
                    DWORD dwWidth;
                    DWORD dwHeight;
                } AVIStreamHeader;
            } StreamHeaderChunk;
            struct {
                char ckID[4];
                DWORD ckSize;
                struct {
                    DWORD biSize;
                    LONG biWidth;
                    LONG biHeight;
                    WORD biPlanes;
                    WORD biBitCount;
                    DWORD biCompression;
                    DWORD biSizeImage;
                    LONG biXPelsPerMeter;
                    LONG biYPelsPerMeter;
                    DWORD biClrUsed;
                    DWORD biClrImportant;
                } BITMAPINFO;
            } StreamFormatChunk;

        } StreamHeaderList1;
        struct {
            char magic[4];  /* "LIST" */
            uint32_t listSize;
            char listType[4]; //strl
            struct {
                char ckID[4];
                DWORD ckSize;
                struct {
                    FOURCC fccType[4];
                    FOURCC fccHandler[4];
                    DWORD dwFlags;
                    WORD wPriority;
                    WORD wLanguage;
                    DWORD dwInitialFrames;
                    DWORD dwScale;
                    DWORD dwRate;
                    DWORD dwStart;
                    DWORD dwLength;
                    DWORD dwSuggestedBufferSize;
                    DWORD dwQuality;
                    DWORD dwSampleSize;
                    DWORD dwWidth;
                    DWORD dwHeight;
                } AVIStreamHeader;
            } StreamHeaderChunk;
            struct {
                char ckID[4];
                DWORD ckSize;
                struct {
                    WORD wFormatTag;
                    WORD nChannels;
                    DWORD nSamplesPerSec;
                    DWORD nAvgBytesPerSec;
                    WORD nBlockAlign;
                    WORD wBitsPerSample;
                    WORD cbSize;
                } WAVEFORMATEX;
            } StreamFormatChunk;

        } StreamHeaderList2;
    } HeaderList;
    struct {
        char magic[4];  /* "LIST" */
        uint32_t listSize;
        char listType[4]; //movi
    } MovieList;
};


static inline void skipJunkData(FILE *file) {
    while (1) {
        char magic[4];
        uint32_t size;
        fread(&magic, sizeof magic, 1, file);
        fread(&size, sizeof size, 1, file);
        if (memcmp(magic, "JUNK", 4) == 0) {
            fseek(file, size, SEEK_CUR);
        } else {
            fseek(file, -(8), SEEK_CUR);
            break;
        }
    }
}

static inline void toNextList(FILE *file) {
    while (1) {
        char magic[4];
        uint32_t size;
        fread(&magic, sizeof magic, 1, file);
        fread(&size, sizeof size, 1, file);
        if (memcmp(magic, "LIST", 4) == 0) {
            fseek(file, -(8), SEEK_CUR);
            break;
        } else {
            fseek(file, size, SEEK_CUR);
        }
    }
}

static inline void toNextMovieList(FILE *file) {
    while (1) {
        char magic[4];
        uint32_t size;
        char type[4];
        fread(&magic, sizeof magic, 1, file);
        fread(&size, sizeof size, 1, file);
        fread(&type, sizeof type, 1, file);
        if (memcmp(magic, "LIST", 4) == 0 && memcmp(type, "movi", 4) == 0) {
            fseek(file, -(12), SEEK_CUR);
            break;
        } else {
            fseek(file, size - 4, SEEK_CUR);
        }
    }
}

static inline void alignToWORD(FILE *file) {

    if (ftell(file) % sizeof(WORD) != 0) {
        fseek(file, 1, SEEK_CUR);
    }

}




TaskHandle_t handle_taskFlush;
static uint8_t jpegBuffer[1024*50];
static int size;

void taskFlush(void *parm) {

    while (1) {
        ESP_LOGI("LCD", "LCD");
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        jpeg_decode(jpegBuffer,size);
        esp_lcd_panel_draw_bitmap(lcd_panel_handle, 0, 0, LCD_WIDTH, LCD_HEIGHT, frameBuffer);

    }

}


void app_main(void) {
    xTaskCreatePinnedToCore(taskFlush, "taskFlush", 4 * 1024, NULL, 5, &handle_taskFlush, 1);

    startTaskMonitor(10000);
    init_sd();
    init_lcd();

    FILE *file = fopen("/sdcard/video.avi", "rb");
    struct AVIFile avi;
    long nextListPos;
    fread(&avi, offsetof(struct AVIFile, HeaderList), 1, file);
    if (memcmp(avi.magic, "RIFF", 4) == 0 && memcmp(avi.fileType, "AVI", 3) == 0) {
        printf("good avi\n");
    }

    skipJunkData(file);

    fread(&avi.HeaderList, 12, 1, file);
    if (memcmp(avi.HeaderList.listType, "hdrl", 4) == 0) {
        printf("HeaderList.listType ok\n");
    }
    skipJunkData(file);
    nextListPos = ftell(file) - 4 + avi.HeaderList.listSize;

    fread(&avi.HeaderList.AVIHeader, sizeof avi.HeaderList.AVIHeader, 1, file);
    if (memcmp(avi.HeaderList.AVIHeader.ckID, "avih", 4) == 0) {
        printf("HeaderList.AVIHeader.ckID ok\n");
    }
    skipJunkData(file);

    fread(&avi.HeaderList.StreamHeaderList1, 12, 1, file);
    if (memcmp(avi.HeaderList.StreamHeaderList1.listType, "strl", 4) == 0) {
        printf("HeaderList.StreamHeaderList1.listType ok\n");
    }
    skipJunkData(file);

    fread(&avi.HeaderList.StreamHeaderList1.StreamHeaderChunk,
          sizeof avi.HeaderList.StreamHeaderList1.StreamHeaderChunk, 1, file);
    if (memcmp(avi.HeaderList.StreamHeaderList1.StreamHeaderChunk.ckID, "strh", 4) == 0) {
        printf("HeaderList.StreamHeaderList1.StreamHeaderChunk.ckID ok\n");
    }
    alignToWORD(file);
    skipJunkData(file);

    fread(&avi.HeaderList.StreamHeaderList1.StreamFormatChunk,
          sizeof avi.HeaderList.StreamHeaderList1.StreamFormatChunk, 1, file);
    if (memcmp(avi.HeaderList.StreamHeaderList1.StreamFormatChunk.ckID, "strf", 4) == 0) {
        printf("HeaderList.StreamHeaderList1.StreamFormatChunk.ckID ok\n");
    }
    alignToWORD(file);
    skipJunkData(file);


    toNextList(file);
    fread(&avi.HeaderList.StreamHeaderList2, 12, 1, file);
    if (memcmp(avi.HeaderList.StreamHeaderList2.listType, "strl", 4) == 0) {
        printf("HeaderList.StreamHeaderList2.listType ok\n");
    }
    skipJunkData(file);

    fread(&avi.HeaderList.StreamHeaderList2.StreamHeaderChunk,
          sizeof avi.HeaderList.StreamHeaderList2.StreamHeaderChunk, 1, file);
    if (memcmp(avi.HeaderList.StreamHeaderList2.StreamHeaderChunk.ckID, "strh", 4) == 0) {
        printf("HeaderList.StreamHeaderList2.StreamHeaderChunk.ckID ok\n");
    }
    alignToWORD(file);
    skipJunkData(file);

    fread(&avi.HeaderList.StreamHeaderList2.StreamFormatChunk,
          sizeof avi.HeaderList.StreamHeaderList2.StreamFormatChunk, 1, file);
    if (memcmp(avi.HeaderList.StreamHeaderList2.StreamFormatChunk.ckID, "strf", 4) == 0) {
        printf("HeaderList.StreamHeaderList2.StreamFormatChunk.ckID ok\n");
    }
    fseek(file, avi.HeaderList.StreamHeaderList2.StreamFormatChunk.WAVEFORMATEX.cbSize, SEEK_CUR);
    alignToWORD(file);


    fseek(file, nextListPos, SEEK_SET);
    toNextMovieList(file);
    fread(&avi.MovieList, 12, 1, file);
    if (memcmp(avi.MovieList.listType, "movi", 4) == 0) {
        printf("avi.MovieList.listType ok\n");
    }

    long end = ftell(file) + avi.MovieList.listSize;
    do {
        struct Chunk chunk;
        fread(&chunk, 8, 1, file);
//        printf("%c %c %c %c\n", chunk.ckID[0], chunk.ckID[1], chunk.ckID[2], chunk.ckID[3]);
//        printf("%ld\n", ftell(file));
        if (memcmp(chunk.ckID, "00dc", 4) == 0) {
            static int dc =0;
//            printf("%d dc\n",dc++);
//            fseek(file, chunk.ckSize, SEEK_CUR);
            fread(jpegBuffer, chunk.ckSize,1,file);
            size=chunk.ckSize;


            xTaskNotifyGive(handle_taskFlush);


        } else if(memcmp(chunk.ckID, "01wb", 4) == 0) {
            static int wb =0;
//            printf("                                           %d wb\n",wb++);
            fseek(file, chunk.ckSize, SEEK_CUR);
        }
        else {
            fseek(file, chunk.ckSize, SEEK_CUR);
        }
        alignToWORD(file);
    } while (ftell(file) <= end);





//    uint8_t fps_count = 0;
//    timeProbe_t fps;
//    timeProbe_start(&fps);
//    while (1) {
//        fps_count++;
//        if (fps_count == 0) {
//            ESP_LOGI("fps", "fps: %f", 256.0f * 1000.0f / (timeProbe_stop(&fps) / 1000.0));
//            timeProbe_start(&fps);
//        }
//        jpeg_decode();
//
//        esp_lcd_panel_draw_bitmap(lcd_panel_handle,0,0,240,240,frameBuffer);
//
//        vTaskDelay(pdMS_TO_TICKS(1));
//
//    }


}