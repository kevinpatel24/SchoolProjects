#include <vector>
#include <stdlib.h> 
#include <sys/time.h>
#include <iostream>
#include <random>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <cmath>
#include <numeric>
using namespace std;

double prims();
void generate_dim();
double euclid(std::vector<double> one, std::vector<double> two);
std::vector <std::vector<double> > vertex_locations;
int vertices; 
int dimensions;
int trials;

//    Return the current absolute time as a real number of seconds.
double tstamp() {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    return now.tv_sec + now.tv_nsec * 1e-9;
}

//    Return the number of seconds that have elapsed since `time`.
double elapsed(double time) {
    return tstamp() - time;
}

//Helpers for generating random values between 0 and 1
std::random_device rd;
std::default_random_engine eng(rd());
std::uniform_real_distribution<float> distr(0, 1);


//Heap implemenation reference: https://www.geeksforgeeks.org/binary-heap/
class Heap{
    //this array keeps track of the heap; heap[0] is the root
    std::vector<std::pair<int,double> > heap;
    int *vertex_to_heap; //matches our vertices to their location in the heap
    //we want to mantain just one node in the heap for each vertex
    public:
    Heap(int num_vertices){
        //Create new array of vertices size and store -1 in each slot 
        vertex_to_heap = new int [num_vertices];
        for(int i = 0; i < num_vertices; i++)
        {
            vertex_to_heap[i] = -1;
        }
    }

    bool isEmpty(){
        return heap.size() == 0;
    }

    //Returns Child to the left
    int left(int v){
        return 2*v+1;
    }

    //Returns chils to the right
    int right(int v){
        return 2*(v+1);
    }
    
    //Returns parent of this vertice
    int parent(int v){
        return (v-1)/2;
    }


    //Deletes shortest edge; takes the last element in the heap list, puts at the root and "heapifies" the heap
    pair<int, double> deletemin(){
        pair<int, double> min = heap[0];
        vertex_to_heap[heap[0].first] = -1;
        heap[0] = heap[heap.size()-1];
        heap.pop_back();
        min_heapify(0);
        return min;
    }
    //Inserts an element into the heap. v is the destination and dist is the edge weight
    void insert(int v, double dist){
        if(vertex_to_heap[v] != -1){
            heap[vertex_to_heap[v]] = pair<int,double>(v,dist);
            swim_up(vertex_to_heap[v]);
        } 
        else{
            heap.push_back(pair<int,double>(v,dist));
            vertex_to_heap[v] = heap.size()-1;
            swim_up(vertex_to_heap[v]);
        }
    }

    //When we put a new node into the heap or change the value of some element in the heap, we "swim" the node up to its proper position
    void swim_up(int newNode){ //newNode is the index in the heap
        //Gives us what the parent node will be for this new node.
        int p = parent(newNode);
        
        int c = newNode;
        while(p >= 0 && heap[p].second > heap[c].second){
            switch_place(p,c);
            c = p;
            p = parent(c);
            if(c == 0){ //cant swim up anymore
                break;
            }
        }
    }

    //Switches the places of v,w in the heap. v,w are heap indices
    void switch_place(int v, int w){
        std::pair<int,double> temp1 = heap[v];
        std::pair<int,double> temp2 = heap[w];
        heap[v] = temp2;
        heap[w] = temp1;
        vertex_to_heap[temp2.first] = v;
        vertex_to_heap[temp1.first] = w;
    }

    //Basically adjusts the heap by looking at the tree below the node init and calling itself recursively until init is in the right spot
    void min_heapify(int init){
        int l = left(init);
        int r = right(init);
        int min = init;
        int t = min;
        if(l < heap.size()){
            if (heap[l].second < heap[min].second){
                min = l;
            }
        }
        if(r < heap.size()){
            if (heap[r].second < heap[min].second){
                min = r;
            }
        }
        if(t != min){
            switch_place(init,min);
            min_heapify(min);
        }
    }
};

int main(int argc, char* argv[]) {
    vertices = atoi(argv[2]);
    dimensions = atoi(argv[4]);
    trials = atoi(argv[3]);    

    double max = 0;

        if(dimensions == 0){
            dimensions++;
        }
        double overall_start_time = tstamp();
        for(int i = 0; i < trials; i++)
        {
            vertex_locations = std::vector<std::vector<double> > (vertices);
            generate_dim();
            double result = prims();
            max += result;   
        }
        double end_time = elapsed(overall_start_time);

        if(dimensions == 1){
            dimensions--;
        }

        printf("Average cost: %f, Vertices: %i, Number of trials: %i, Dimmensions: %i\n", max/trials, vertices, trials, dimensions);
    
}


//Prims algorithm which will utilize our binary heap class above
double prims(){
    //Create new visited array and heap of unvisited vertices
    int visited [vertices];
    Heap H = Heap(vertices);

    
    //Create new array for tracking distances to source for each vertice and mark all vertices as unvisited in visited array
    double dist[vertices];
    for(int i = 0; i < vertices; i ++){
        dist[i] = 20.0; //no distance is > 10, functionally infinite
        visited[i] = 0;
    }
    
    //Distance from source to source is set to 0 and inserted in the heap
    dist[0] = 0;
    
    H.insert(0,0.0);

    double MST_sum = 0;
    int count = 0;

    //Delete the minimum distance vertice from the heap until the heap is emoty
    while(!H.isEmpty()){
        pair<int,double> v = H.deletemin();

        if(dimensions != 1){
            MST_sum += std::sqrt(v.second);
        }
        else{
            MST_sum += v.second;
        }
        visited[v.first] = 1;
        for(int i = 0 ; i < vertices; i ++){
            if(visited[i] == 0 && i != v.first){
                double currDist = euclid(vertex_locations[v.first],vertex_locations[i]);
                if(currDist < dist[i]) {
                    dist[i] = currDist;
                    H.insert(i,dist[i]);
                }
            }
        }
    }
    return MST_sum;
}

/**
 * Finds the eucliden distance between two coordinates
 * @param one a coordinate of length `dimension'
 * @param two a coordinate of length `dimension'
 * @return returns the Euclidean distance between two coordinates
 */
double euclid(std::vector<double> one, std::vector<double> two){
    double sum = 0;
    if(dimensions == 1){
        return distr(eng);
    }
    
    for(int j = 0; j < dimensions; j++){
        double diff = one[j] - two[j];
        sum += diff * diff;
    }
    return sum;
}

/**
 * Generates edges for a complete graph. Vertex locations are based off a random uniform distribution
 * over a unit square/cube/hypercube depending on the number of dimensions. The edge weights are the
 * eucliden distance between the vertices.
 */ 
void generate_dim(){
    for(int i = 0; i < vertices; i++){
        std::vector<double> vec;
        vec = std::vector<double> (dimensions);
        for(int j = 0; j < dimensions; j++){
            vec[j] = distr(eng);
        }
        vertex_locations[i] = vec;
    }
}