#include "JPEGDEC.h"
#include "jpeg.inl"
#include "jpeg.h"

JPEGDEC jpeg;

extern const uint8_t _binary_img_jpg_start[] asm("_binary_img_jpg_start");
extern const uint8_t _binary_img_jpg_end[]   asm("_binary_img_jpg_end");


static int a=0;
int JPEGDraw(JPEGDRAW *pDraw)
{
    printf("draw %d\n",a++);
    return 1;
} /* JPEGDraw() */


extern "C" void jpeg_decode(){
    // Open a large JPEG image stored in FLASH memory (included as thumb_test.h)
    // This image is 12 megapixels, but has a 320x240 embedded thumbnail in it
    if (jpeg.openRAM(const_cast<uint8_t *>(_binary_img_jpg_start), _binary_img_jpg_end - _binary_img_jpg_start, JPEGDraw))
    {
        printf("Successfully opened JPEG image\n");
        printf("Image size: %d x %d, orientation: %d, bpp: %d\n", jpeg.getWidth(),
                      jpeg.getHeight(), jpeg.getOrientation(), jpeg.getBpp());
        if (jpeg.hasThumb())
            printf("Thumbnail present: %d x %d\n", jpeg.getThumbWidth(), jpeg.getThumbHeight());
        jpeg.setPixelType(RGB565_BIG_ENDIAN); // The SPI LCD wants the 16-bit pixels in big-endian order
        // Draw the thumbnail image in the middle of the display (upper left corner = 120,100) at 1/4 scale
        if (jpeg.decode(0,0, 0))
        {
            printf("111111111111111111111111111111111111111\n");
        }
        jpeg.close();
    }
}