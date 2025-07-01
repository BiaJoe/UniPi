#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ABS(x) ((x) < 0 ? -(x) : (x))

#define GRID_SIZE_X 200
#define GRID_SIZE_Y 50



// --- Slope heuristic method
void compute_vx_vy_slope(int xA, int yA, int xB, int yB, int cpt, int *vx, int *vy) {
    float dx = (float)ABS(xB - xA);
    float dy = (float)ABS(yB - yA);
    float m = (dx != 0) ? dy / dx : 0.0f;
    float fx = (dx != 0) ? cpt / (m + 1.0f) : 0.0f;
    float fy = cpt - fx;
    *vx = (int)fx;
    *vy = (int)fy;
    if (xB - xA < 0) *vx = -*vx;
    if (yB - yA < 0) *vy = -*vy;
}

// --- Normalized method
void compute_vx_vy_normalized(int xA, int yA, int xB, int yB, int cpt, int *vx, int *vy) {
    float dx = (float)(xB - xA);
    float dy = (float)(yB - yA);
    float sum = ABS(dx) + ABS(dy);
    if (sum == 0.0f) {
        *vx = 0;
        *vy = 0;
        return;
    }
    float fx = (ABS(dx) / sum) * cpt;
    float fy = (ABS(dy) / sum) * cpt;
    *vx = (int)fx;
    *vy = (int)fy;
    if (dx < 0) *vx = -*vx;
    if (dy < 0) *vy = -*vy;
}

void compute_vx_vy_bresenham(int xA, int yA, int xB, int yB, int cpt, int *vx, int *vy) {
    static int dx, dy, sx, sy, err;
    static int initialized = 0;

    if (!initialized) {
        dx = ABS(xB - xA);
        dy = ABS(yB - yA);
        sx = (xA < xB) ? 1 : -1;
        sy = (yA < yB) ? 1 : -1;
        err = dx - dy;
        initialized = 1;
    }

    int newX = xA;
    int newY = yA;

    for (int i = 0; i < cpt; i++) {
        if (newX == xB && newY == yB) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            newX += sx;
        }
        if (e2 < dx) {
            err += dx;
            newY += sy;
        }
    }

    *vx = newX - xA;
    *vy = newY - yA;
}

void compute_vx_vy_bresenham_stateless(int xA, int yA, int xB, int yB, int cpt, int *vx, int *vy) {
    int dx = ABS(xB - xA);
    int dy = ABS(yB - yA);
    int sx = (xA < xB) ? 1 : -1;
    int sy = (yA < yB) ? 1 : -1;
    int err = dx - dy;

    int newX = xA;
    int newY = yA;

    for (int i = 0; i < cpt; i++) {
        if (newX == xB && newY == yB) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            newX += sx;
        }
        if (e2 < dx) {
            err += dx;
            newY += sy;
        }
    }

    *vx = newX - xA;
    *vy = newY - yA;
}

int manhattan_distance(int xA, int yA, int xB, int yB) {
    return ABS(xA - xB) + ABS(yA - yB);
}

// --- Run method and record visited cells
void run_method(
    const char *method_name,
    void (*compute_vx_vy)(int,int,int,int,int,int*,int*),
    int xA_start, int yA_start, int xB, int yB, int cpt,
    char grid[GRID_SIZE_Y][GRID_SIZE_X]
) {
    int xA = xA_start;
    int yA = yA_start;

    // Initialize grid
    for (int i = 0; i < GRID_SIZE_Y; i++)
        for (int j = 0; j < GRID_SIZE_X; j++)
            grid[i][j] = '.';

    // Mark target and start
    if (xB >= 0 && xB < GRID_SIZE_X && yB >= 0 && yB < GRID_SIZE_Y)
        grid[yB][xB] = 'T';
    if (xA >= 0 && xA < GRID_SIZE_X && yA >= 0 && yA < GRID_SIZE_Y)
        grid[yA][xA] = 'S';

    while (1) {
        int dist = manhattan_distance(xA, yA, xB, yB);
        if (dist <= cpt) {
            // final jump to target
            if (xA != xB || yA != yB) {
                if (xA >= 0 && xA < GRID_SIZE_X && yA >= 0 && yA < GRID_SIZE_Y)
                    grid[yA][xA] = '#';
            }
            xA = xB;
            yA = yB;
            break;
        }
        int vx, vy;
        compute_vx_vy(xA, yA, xB, yB, cpt, &vx, &vy);
        if (xA >= 0 && xA < GRID_SIZE_X && yA >= 0 && yA < GRID_SIZE_Y)
            grid[yA][xA] = '*';
        xA += vx;
        yA += vy;
    }
}

// --- Print grid to screen
void print_grid(const char *title, char grid[GRID_SIZE_Y][GRID_SIZE_X]) {
    printf("\n== %s ==\n", title);
    for (int y = 0; y < GRID_SIZE_Y; y++) {
        for (int x = 0; x < GRID_SIZE_X; x++) {
            putchar(grid[y][x]);
        }
        putchar('\n');
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Usage: %s xA yA xB yB cpt\n", argv[0]);
        return 1;
    }

    int xA = atoi(argv[1]);
    int yA = atoi(argv[2]);
    int xB = atoi(argv[3]);
    int yB = atoi(argv[4]);
    int cpt = atoi(argv[5]);

    printf("Start: (%d,%d) -> Target: (%d,%d), cpt=%d\n", xA, yA, xB, yB, cpt);

    char grid_bresenham[GRID_SIZE_X][GRID_SIZE_X];
    char grid_bresenham_stateless[GRID_SIZE_X][GRID_SIZE_X];


    run_method("Bresenham method", compute_vx_vy_bresenham, xA, yA, xB, yB, cpt, grid_bresenham);
    run_method("Bresenham stateless method", compute_vx_vy_bresenham_stateless, xA, yA, xB, yB, cpt, grid_bresenham_stateless);


    print_grid("Bresenham method", grid_bresenham);
    print_grid("Bresenham stateless method", grid_bresenham_stateless);


    return 0;
}