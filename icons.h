// https://diyusthad.com/image2cpp
const unsigned char ethereum [] PROGMEM = {
  0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x0f, 0x00, 0x1f, 0x80, 0x1f, 0x80, 0x1f, 0xc0, 0x3f, 0xe0, 
  0x7f, 0xe0, 0xff, 0xf8, 0xff, 0xf8, 0xbf, 0xc8, 0x0f, 0x00, 0x72, 0x60, 0x3d, 0xc0, 0x1f, 0x80, 
  0x0f, 0x80, 0x0f, 0x00, 0x06, 0x00, 0x00, 0x00
};
// display.drawBitmap(0, 0, ethereum, 13, 20, 1);

const unsigned char bitcoin [] PROGMEM = {
  0x01, 0xf8, 0x00, 0x07, 0xfe, 0x00, 0x1f, 0xff, 0x80, 0x3f, 0xff, 0xc0, 0x3f, 0xaf, 0xc0, 0x7e, 
  0x0f, 0xe0, 0x7f, 0x07, 0xe0, 0xff, 0x33, 0xf0, 0xff, 0x33, 0xf0, 0xfe, 0x03, 0xf0, 0xfe, 0x67, 
  0xf0, 0xfe, 0x73, 0xf0, 0xf8, 0x73, 0xf0, 0x7e, 0x03, 0xe0, 0x7e, 0xbf, 0xe0, 0x3f, 0xbf, 0xc0, 
  0x3f, 0xff, 0xc0, 0x1f, 0xff, 0x80, 0x07, 0xfe, 0x00, 0x01, 0xf8, 0x00
};
// display.drawBitmap(0, 0, bitcoin, 20, 20, 1);
