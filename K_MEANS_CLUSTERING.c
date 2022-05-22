#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include "omp.h"



const int MAX = 9;

struct Point 
{
    double x;
    double y;
};

struct Cluster 
{
    struct Point* members;
    struct Point* old_members;
    struct Point centroid;
    int member_count;
    int old_member_count;
};

int clusters_count;
int points_count;
double* random_points;
struct Point* points;
struct Cluster* clusters;


void generateRandomPoints() 
{
    int i;
    double random_value1;
    srand(time(0));
    for (i = 0; i < (clusters_count * 2); i++) 
    {
        random_value1 = (double)(rand() % MAX);
        random_points[i] = random_value1;
    }
}

double calc_distance(struct Point a, struct Point b) 
{
    return sqrt(pow(b.x - a.x, 2) + pow(b.y - a.y, 2));
}

//return 0 if not identical
int compare2PointArrays(struct Point* a1, struct Point* a2, int c1, int c2) 
{
    if (c1 == 0) 
    {
        //printf("\nempty cluster\n");
        return 1;
    }
    else if (c2 == 0) 
    {
        //printf("\nHad No Prev\n");
        return 1;
    }
    if (c1 != c2) 
    {
        return 0;
    }
    int i = 0;
    for (; i < c1; i++) 
    {
        //printf("Compare:- %f %f : %f %f\n", a1[i].x, a1[i].y, a2[i].x, a2[i].y);
        if (a1[i].x != a2[i].x || a1[i].y != a2[i].y) 
        {
            return 0;
        }
    }
    return 1;
}

int getFileSize() 
{
    FILE* file_pointer;
    char line[15];
    file_pointer = fopen("points.txt", "r");
    int fsize = 0;
    while (!feof(file_pointer))
    {
        fgets(line, 15, file_pointer);
        fsize++;
    }

    fclose(file_pointer);
    return fsize;
}

void openFile() 
{
    const int fsize = getFileSize();
    points_count = fsize;
    FILE* fpointer;
    fpointer = fopen("points.txt", "r");


    points = calloc(points_count, sizeof(struct Point));

    char line[15];

    double* x_points = calloc(fsize, sizeof(double));
    double* y_points = calloc(fsize, sizeof(double));
    double x = 0.0;
    double y = 0.0;
    int c = 0;
    while (!feof(fpointer))
    {

        char* current_x = calloc(10, sizeof(char)), * current_y = calloc(10, sizeof(char));
        fgets(line, 15, fpointer);
        int flag = 0;
        int j;
        int jx = 0;
        int jy = 0;
        for (j = 0; j < 15; j++) 
        {
            if (line[j] == '(') { continue; }
            if (line[j] == ',')
            {
                //if (line[j + 1] == '\n' || line[j + 1] == ' ') { break; }
                flag = 1;
                continue;
            }
            if (line[j] == ')') 
            {
                break;
            }
            if (!flag) 
            {
                current_x[jx++] = line[j];
            }
            else 
            {
                current_y[jy++] = line[j];
            }
        }
        current_x += '\0';
        current_y += '\0';
        x = atof(current_x);
        y = atof(current_y);

        x_points[c] = x;
        y_points[c] = y;

        //printf("%f %f \n",x_points[c],y_points[c]);

        c++;
    }

    fclose(fpointer);
    int i;
    for (i = 0; i < points_count; i++) 
    {
        // printf("%f %f\n", x_points[i], y_points[i]);
        points[i].x = x_points[i];
        points[i].y = y_points[i];
    }
}

int main(int argc, char* argv[]) 
{
    openFile();

    int np = 0;
#pragma omp parallel default(shared) private(np)
    {
        np = omp_get_num_threads();
        clusters_count = np;

    }
    printf("%d\n", clusters_count);

    //Entered by user


    //Allocate space for clusters Array
    clusters = calloc(clusters_count, sizeof(struct Cluster));
    random_points = calloc(clusters_count * 2, sizeof(double));
    generateRandomPoints();
    //Generate random centroids for clusters
    int j = 0;
    int r = 0;
    for (; j < clusters_count; j++) 
    {
        clusters[j].centroid.x = random_points[r++];
        clusters[j].centroid.y = random_points[r++];
        clusters[j].member_count = 0;
        clusters[j].old_member_count = 0;
        clusters[j].members = calloc(points_count, sizeof(struct Point));
        clusters[j].old_members = calloc(points_count, sizeof(struct Point));
    }

    //Output initial centroids
    for (j = 0; j < clusters_count; j++)
    {
        printf("Cluster %d Centroid:( %f , %f )\n", j, clusters[j].centroid.x, clusters[j].centroid.y);
    }

    //Output points in file
    int i;
    for (i = 0; i < points_count; i++) 
    {
        //printf("%f %f \n", points[i].x, points[i].y);
        //printf("%f\n",generateRandomPoint());
    }

    //ALGORITHM
    int tnum = 0;

    for (i = 0; i < points_count; i++) 
    {
        struct Point currentPoint = points[i];
        double min_distance = DBL_MAX;
        int cluster_index = -1;
        /* Run by threads */
        double* distances = calloc(clusters_count,sizeof(double));
        #pragma omp parallel
        {
            tnum = omp_get_thread_num();
            #pragma omp for
            for (j = 0; j < clusters_count; j++)
            {
                double distance = calc_distance(currentPoint, clusters[j].centroid);
                distances[j] = distance;
                printf("Thread Rank: %d -> Calculated distance from Point: %.2f,%.2f to centroid %.2f ,%.2f\n", tnum, points[i].x, points[i].y, clusters[j].centroid.x, clusters[j].centroid.y);
            }
        }
        for (j = 0; j < clusters_count; j++)
        {
            double distance = distances[j];
            if (distance < min_distance) 
            {
                min_distance = distance;
                cluster_index = j;
            }
        }

        int oldCount = clusters[cluster_index].member_count;
        clusters[cluster_index].member_count++;
        clusters[cluster_index].old_member_count++;
        clusters[cluster_index].members[oldCount].x = currentPoint.x;
        clusters[cluster_index].members[oldCount].y = currentPoint.y;
        clusters[cluster_index].old_members[oldCount].x = currentPoint.x;
        clusters[cluster_index].old_members[oldCount].y = currentPoint.y;
    }


    printf("Initial State");
    for (i = 0; i < clusters_count; i++)
    {
        struct Cluster current = clusters[i];
        for (j = 0; j < current.member_count; j++)
        {
            printf("Cluster no %d : %f %f\n", i, current.members[j].x, current.members[j].y);
        }
    }


    int loop = 1;
    while (loop)
    {

        int change = 0;
        //Re-Calculating the Centroids
#pragma omp parallel
        {
            tnum = 0;
            int k;
#pragma omp for
            for (k = 0; k < clusters_count; ++k) 
            {
                tnum = omp_get_thread_num();
                double centX = 0.0;
                double centY = 0.0;
                int l;
                for (l = 0; l < clusters[k].member_count; ++l) {
                    centX += clusters[k].members[l].x;
                    centY += clusters[k].members[l].y;
                }
                if (clusters[k].member_count == 0) {
                    continue;
                }
                centX = centX / clusters[k].member_count;
                centY = centY / clusters[k].member_count;
                clusters[k].centroid.x = centX;
                clusters[k].centroid.y = centY;
                printf("Thread Rank: %d Recalculated Centroid for Cluster Number: %d\n", tnum, k);
            }
        }
        //Clearing the Current Members!

        for (j = 0; j < clusters_count; j++)
        {
            int l;
            for (l = 0; l < clusters[j].member_count; ++l)
            {
                clusters[j].members[l].x = 0.0;
                clusters[j].members[l].y = 0.0;
            }
            clusters[j].member_count = 0;

        }

        //TO DO
        //Re-Allocating..
        for (i = 0; i < points_count; i++) 
        {
            struct Point currentPoint = points[i];
            double min_distance = DBL_MAX;
            int cluster_index = -1;
            /* Run by threads */
            double* distances = calloc(clusters_count, sizeof(double));
            /* Run by threads */
            #pragma omp parallel
            {
                tnum = omp_get_thread_num();
                #pragma omp for
                for (j = 0; j < clusters_count; j++)
                {
                    double distance = calc_distance(currentPoint, clusters[j].centroid);
                    distances[j] = distance;
                    printf("Thread Rank: %d -> Calculated distance from Point: %.2f,%.2f to centroid %.2f ,%.2f\n", tnum, points[i].x, points[i].y, clusters[j].centroid.x, clusters[j].centroid.y);
                }
            }
            for (j = 0; j < clusters_count; j++)
            {
                double distance = distances[j];
                if (distance < min_distance) 
                {
                    min_distance = distance;
                    cluster_index = j;
                }
            }

            int oldCount = clusters[cluster_index].member_count;
            clusters[cluster_index].member_count++;
            clusters[cluster_index].members[oldCount].x = currentPoint.x;
            clusters[cluster_index].members[oldCount].y = currentPoint.y;
            
        }

        printf("After Re-Allocating\n");
        for (i = 0; i < clusters_count; i++)
        {
            struct Cluster current = clusters[i];
            printf("Centroid of cluster no %d is %.2f %.2f \n", i, current.centroid.x, current.centroid.y);

            for (j = 0; j < current.member_count; j++)
            {
                printf("Cluster no %d : %f %f\n", i, current.members[j].x, current.members[j].y);
            }
        }

        //Comparing Arrays!
        for (i = 0; i < clusters_count; i++) 
        {
            if (!compare2PointArrays(clusters[i].members, clusters[i].old_members, clusters[i].member_count, clusters[i].old_member_count)) 
            {
                change = 1;
                clusters[i].old_members = calloc(points_count, sizeof(struct Point));
                for (j = 0; j < clusters[i].member_count; j++)
                {
                    clusters[i].old_members[j].x = clusters[i].members[j].x;
                    clusters[i].old_members[j].y = clusters[i].members[j].y;
                }
                clusters[i].old_member_count = clusters[i].member_count;
            }
        }


        if (change == 0)
        {
            printf("Centroid re-calculated but no change detected\n");
            break;
        }
    }

    free(clusters);
    free(points);
    return 0;
}