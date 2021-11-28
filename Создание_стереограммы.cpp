#define _USE_MATH_DEFINES
struct Sphere {

    Vec3f center;

    float radius;

};



bool sphere_ray_intersect(const Sphere &sphere, const Vec3f &orig, const Vec3f &dir, float &t0) {

    Vec3f L = sphere.center - orig;

    float tca = L*dir;

    float d2 = L*L - tca*tca;

    if (d2 > pow(sphere.radius, 2)) return false;

    float thc = sqrtf(pow(sphere.radius, 2) - d2);

    t0 = tca - thc;

    float t1 = tca + thc;

    if (t0 < 0) t0 = t1;

    if (t0 < 0) return false;

    return true;

}



float scene_intersect(const Vec3f &orig, const Vec3f &dir, const std::vector<Sphere> &spheres) {

    float spheres_dist = std::numeric_limits<float>::max();

    for (size_t i=0; i<spheres.size(); i++) {

        float dist_i;

        if (sphere_ray_intersect(spheres[i], orig, dir, dist_i) && dist_i < spheres_dist) {

            spheres_dist = dist_i;

        }

    }



    float checkerboard_dist = std::numeric_limits<float>::max();

    if (fabs(dir.y)>1e-3) {

        float d = -(orig.y+4)/dir.y; // плоскость шахматной доски имеет уравнение y = -4

        Vec3f pt = orig + dir*d;

        if (d>0 && fabs(pt.x)<10 && pt.z<-10 && pt.z>-30) {

            checkerboard_dist = d;

        }

    }



    return std::min(spheres_dist, checkerboard_dist);

}



void compute_depthmap(const size_t width, const size_t height, const float fov, const float far, const std::vector<Sphere> &spheres, std::vector<float> &zbuffer) {

#pragma omp parallel for

    for (size_t j=0; j<height; j++) {

        for (size_t i=0; i<width; i++) {

            float dir_x = (i + 0.5) - ( width/2.);

            float dir_y = -(j + 0.5) + (height/2.); // это одновременно переворачивает изображение

            float dir_z = -(height/(2.*tan(fov/2.)));

            Vec3f dir = Vec3f{dir_x, dir_y, dir_z}.normalize();

            zbuffer[i+j*width] = scene_intersect(Vec3f{0,0,0}, dir, spheres);

        }

    }



    float minval = std::numeric_limits<float>::max(); // зажимает буфер z дальней плоскостью и нормализуйте его между 0 и 1

    float maxval = -std::numeric_limits<float>::max();

    for (size_t i=0; i<height*width; ++i) {

        minval = std::min(minval, zbuffer[i]);

        maxval = std::max(maxval, std::min(zbuffer[i], far));

    }

    for (size_t i=0; i<height*width; ++i)

        zbuffer[i] = 1-(std::min(zbuffer[i],far)-minval)/(maxval-minval);

}




int parallax(const float z) {

    const float eye_separation = 400.; // межзрачковое расстояние в пикселях

    const float mu = .33; // если дальняя плоскость находится на расстоянии D за экраном, то ближняя плоскость находится на расстоянии mu*D перед дальней плоскостью

    return static_cast<int>(eye_separation*((1.-z*mu)/(2.-z*mu))+.5);

}




size_t uf_find(std::vector<size_t> &same, size_t x) {

    return same[x]==x ? x : uf_find(same, same[x]);

}




void uf_union(std::vector<size_t> &same, size_t x, size_t y) {

    if ((x=uf_find(same, x)) != (y=uf_find(same, y))) same[x] = y;

}




int main() {

    std::vector<Sphere> spheres = { {{-3,0,-16}, 2}, {{-1,-1.5,-12}, 2}, {{1.5,-.5,-18}, 3}, {{7,5,-18}, 4} };

    const size_t width = 1024;

    @@ -79,12 +93,30 @@ int main() {

        std::vector<unsigned char> framebuffer(width*height*3);

        for (size_t j=0; j<height; j++) { // генерируется случайное изображение

            for (size_t i=0; i<width; i++) {

                framebuffer[(i+j*width)*3 + 0] = (rand()%256)*(sin(i*2*M_PI/200)+1)/2; // синус генерирует вертикальные полосы для облегчения фокусировки

                framebuffer[(i+j*width)*3 + 0] = (rand()%256)*(sin(i*2*M_PI/parallax(0))+1)/2; // синус генерирует вертикальные полосы для облегчения фокусировки

                framebuffer[(i+j*width)*3 + 1] = (rand()%256);

                framebuffer[(i+j*width)*3 + 2] = (rand()%256);

            }

        }




        for (size_t j=0; j<height; j++) { // цикл рендеринга автостереограммы

            std::vector<size_t> same(width);

            std::iota(same.begin(), same.end(), 0); // инициализируйте структуру данных "объединение-поиск" (same[i]=i)




            for (size_t i=0; i<width; i++) { // ставятся ограничения

                int par = parallax(zbuffer[i+j*width]);

                int left = i - par/2;

                int right = left + par; // работает лучше, чем i+par/2 для нечетных значений параметров

                if (left>=0 && right<(int)width)

                    uf_union(same, left, right); // левый и правый пиксели будут иметь одинаковый цвет

            }

            for (size_t i=0; i<width; i++) { // устраняются ограничения

                size_t root = uf_find(same, i);

                for (size_t c=0; c<3; c++)

                    framebuffer[(i+j*width)*3+c] = framebuffer[(root+j*width)*3+c];

            }

        }




        std::ofstream ofs("./out.ppm"); // сохраняется буфер кадров в файл

        ofs << "P6\n" << width << " " << height << "\n255\n";

        ofs.write(reinterpret_cast<char *>(framebuffer.data()), 3*height*width);

        ofs.close();



        return 0;

    }
