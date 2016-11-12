#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <sys/time.h>
#include <mpi.h>
#include <vector>
#include <algorithm>

using namespace std;

int N = 36;
int P;
int W;  //jump size 4
int gSamplesPerBlock; //3
int gBlocksize;       //12
int gSamplesize;      //9
int size;

//vector<long> data {16, 2, 17, 24, 33, 28, 30, 1, 0, 27, 9, 25, 34, 23, 19, 18, 11, 7, 21, 13, 8, 35, 12, 29, 6, 3, 4, 14, 22, 15, 32, 10, 26, 31, 20, 5};
vector<long> data;

vector<vector<long>> samples;
vector<long> sample;
vector<long> gPivots;
vector<vector<long>> gPartions;


vector<long> mergedPartion;
vector<long> finalList;


void populateData() {
    srandom(9);
    for (int i = 0; i < N; i++)
        data.push_back(random());
}


void phase1(int rank) {
    int a, b;
    a = rank * gBlocksize;
    b = a + gBlocksize;

    sort(data.begin()+a,data.begin()+b);

    vector<long> payload;
    for(int i = a; i < b; i += W) {
        payload.push_back(data[i]);
    }

//    if(rank == 1) {
//        cout << endl;
//        for(auto i : payload) {
//            printf(" %d ",i);
//        }
//        cout << endl;
//    }


    if(rank != 0)  {
        MPI_Send(&payload[0], payload.size(), MPI_LONG, 0, 0, MPI_COMM_WORLD);
    } else {
        samples.push_back(payload);
        payload.clear();
        for( int i = 1; i < size; i++) {
            payload.resize(gSamplesPerBlock);
            MPI_Status status;
            MPI_Recv(&payload[0], gSamplesPerBlock, MPI_LONG, i, 0, MPI_COMM_WORLD, &status);
            samples.push_back(payload);
            payload.clear();
        }
    }

    if(rank == 0) {
        for(auto i : samples) {
            for (auto x : i) {
                sample.push_back(x);
            }
        }
//        for(auto i : sample) {
//            cout << i << " ";
//        }
    }

}

void phase2(int rank) {
    if(rank == 0) {
        sort(sample.begin(), sample.end());
        gPivots.clear();
        for (int n = 1; n < P; n++) {
            long num = sample[n * P];
            gPivots.push_back(num);
        }
    }
    MPI_Bcast(&gPivots[0], gPivots.size(), MPI_LONG, 0,  MPI_COMM_WORLD);
}

void phase3(int rank) {
    int a, b;
    a = rank * gBlocksize;
    b = a + gBlocksize;
    vector<vector<long>> partions;

    auto start = data.begin() + a;
    auto end = data.begin() + b;
    for (auto pivot : gPivots) {
        auto partEnd = partition(start, end, [&pivot](long x) { return x <= pivot; });
        vector<long> part(start, partEnd);
        partions.push_back(part);
        start = partEnd;
    }
    vector<long> part(start, end);
    partions.push_back(part);

    int sizes[3];

    for(int i = 0 ; i < P; i ++) {
        if(i != rank) {
            int psize = partions[i].size();
            int psizenew;
            MPI_Status status;
            MPI_Send(&psize, 1, MPI_INT, i, 0,MPI_COMM_WORLD);
            printf("[%d] Sending %d to %d\n", rank,psize,i);


            MPI_Recv(&psizenew, 1, MPI_INT, i, 0,MPI_COMM_WORLD, &status);
            printf("[%d] Recived %d from %d\n", rank, psizenew, i);


            sizes[i] = psizenew;
        } else {
            gPartions[i].resize(partions[i].size());
            gPartions[i] = partions[i];
        }
    }
    printf("[%d]---------------\n", rank);

    MPI_Request requests[P-1];
    MPI_Status statuss[P-1];
    int c = 0;
    for(int i = 0 ; i < P; i ++) {
        if(i != rank) {
            MPI_Request request;
            printf("[%d] Perparing Payload to %d\n", rank,i);
            MPI_Isend(&partions[i][0], partions[i].size(), MPI_LONG, i, 0, MPI_COMM_WORLD, &request);
            requests[c] = request;
            printf("[%d] Sending Payload to %d\n", rank,i);
            c++;
        }
    }

    c=0;
    for(int i = 0 ; i < P; i ++) {
        if(i != rank) {
            int psizenew = sizes[i];
            gPartions[i].resize(psizenew);
            MPI_Status status;
            MPI_Recv(&gPartions[i][0], psizenew, MPI_LONG, i, 0,MPI_COMM_WORLD, &status);
            statuss[c] = status;
            printf("[%d] Recieved Partion From: %d \n", rank,i);
            c++;
        }
    }

    printf("[%d]  Done\n", rank);

    MPI_Waitall(P-1,requests,statuss);


//    if(rank == 0) {
//        for(auto i : gPartions) {
//            for(auto g : i) {
//                cout << g << " ";
//            }
//        }
//        cout << "----" << endl;
//    }

}

void phase4() {
    for(int i = 0 ; i < gPartions.size(); i++) {
        int size = mergedPartion.size();
        mergedPartion.insert(mergedPartion.end(), gPartions[i].begin(), gPartions[i].end());
        inplace_merge(mergedPartion.begin(), mergedPartion.begin() + size, mergedPartion.end());
    }
//    if(rank == 1) {
//        for(auto i : mergedPartion) {
//            cout << i << " ";
//        }
//        cout << "----" << endl;
//    }
}

void final(int rank) {

    if(rank == 0) {
        for(int i = 1 ; i < P; i++) {
            vector<long> tmp;
            int fsize;
            MPI_Status status;
            MPI_Recv(&fsize, 1, MPI_INT, i, 0,MPI_COMM_WORLD, &status);

            tmp.resize(fsize);
            MPI_Recv(&tmp[0], fsize, MPI_LONG, i, 0,MPI_COMM_WORLD, &status);

            mergedPartion.insert(mergedPartion.end(), tmp.begin(), tmp.end());
        }
    } else {
            int fsize = mergedPartion.size();
            MPI_Send(&fsize, 1, MPI_INT, 0, 0,MPI_COMM_WORLD);
            MPI_Send(&mergedPartion[0], fsize, MPI_LONG, 0, 0,MPI_COMM_WORLD);
    }


}

void printData() {
    for(int i = 0; i < N; i++ ) {
        cout << data[i] << " ";
    }
}

bool verifySorted() {
    for(int i = 0; i < N -1 ; i++) {
        if( ! (mergedPartion[i] <= mergedPartion[i+1]) ) {
            cout << "NOT SORTED";
            return false;
        }
    }
    return true;
}

int main( int argc, char **argv )
{
    int rank;

    //MP INIT
    MPI_Init( &argc, &argv );
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
//    printf("Rank: %d Size: %d\n",rank,size);

    //Give Data to everyone
    MPI_Bcast(&data[0], data.size(), MPI_LONG, 0,  MPI_COMM_WORLD);
//    printf("[%d] %d\n",rank, data[0]);

    if(size == 1) {
        /* Timing Starts */
        struct timeval t1, t2;
        double elapsedTime;
        gettimeofday(&t1, NULL);
//        quicksort(data, 0, N-1);
        sort(data.begin(),data.end());
        /* Timing Ends */
        gettimeofday(&t2, NULL);
        elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0;      // sec to ms
        elapsedTime += (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
        std::cout << size <<"," << N <<","<< elapsedTime << "\n";
        printData();
        verifySorted();
        return 0;
    }

//  N = 119999997;
    N = 180000000;

    populateData();

    P = size;
    W = N / (P * P);

    gBlocksize = N / size;
    gSamplesize = gBlocksize / W * size;
//    sample = new long[gSamplesize];
    gSamplesPerBlock = gBlocksize / W;

    for(int i = 0; i < gSamplesPerBlock; i++) {
        vector<long> tmp;
        gPartions.push_back(tmp);
    }

//    if (rank == 0 ) {
//        printf("Blocksize %d\n", gBlocksize);
//        printf("samples per block: %d\n", gSamplesPerBlock);
//        printf("sample size: %d\n", gSamplesize);
//    }

    gPivots.resize(gSamplesPerBlock-1);


    if(rank == 0) { printf("PHASE1");}
    phase1(rank);
    if(rank == 0) { printf("PHASE2");}
    phase2(rank);
    if(rank == 0) { printf("PHASE3");}
    phase3(rank);
    if(rank == 0) { printf("PHASE4");}
    phase4();
    if(rank == 0) { printf("FINAL");}
    final(rank);
    if(rank == 0) { printf("FINALIZE");}


    MPI_Finalize();


//    printData();
    if(rank == 0) {
        verifySorted();
    }
    return 0;
}