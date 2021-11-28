private Bitmap GenerateStereoPicture(Bitmap bitmapMask)
{
    // Переводим маску в массив сдвигов
    int w = bitmapMask.Width;
    int h = bitmapMask.Height;
    int[][] mask = new int[w][];
    for (int x = 0; x < w; x++)
    {
        mask[x] = new int[h];
        for (int y = 0; y < h; y++)
            mask[x][y] = bitmapMask.GetPixel(x, y).R / 32;
    }

    // Cоздаем фон
    int s = 100;
    Bitmap stereoImg = GetNewStereoPicture(w + s, h, s);

    // Сдвигаем каждый пиксел
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            if (mask[x][y] > 0)
            {
                Color pixel = stereoImg.GetPixel(x + mask[x][y], y);
                for (int i = x + s; i < w + s; i += s)
                    stereoImg.SetPixel(i, y, pixel);
            }

    return stereoImg;
}