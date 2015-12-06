//! Search simulator -- any number of dims|particles

#include <iostream>
#include <cstdlib>
#include <ctime>
using namespace std;

#define WORLD_SHIFT ((WORLD_BOUNDS-1)/2)
template<int N, int P> struct Power {
  enum { val = N * Power<N, P-1>::val };
};
template<int N> struct Power<N, 0> {
  enum { val = 1 };
};
template<int D> struct WorldCenter{
  enum { val= WORLD_SHIFT*Power<WORLD_BOUNDS,D>::val + WorldCenter<D-1>::val };
};
template<> struct WorldCenter<-1>{
  enum{ val= 0 };
};
template<int D, int dimCache= D> struct Address{
  static int val(const int *position){
    return position[D]*Power<WORLD_BOUNDS,D>::val + Address<D-1, dimCache>::val(position);
  }
};
template<int dimCache> struct Address<-1,dimCache>{
  static int val(const int*) { return WorldCenter<dimCache>::val; }
};

//bit-hacks
/*__attribute__((const)) inline unsigned log2_32(unsigned v) {
  static const unsigned b[] = {0xAAAAAAAA, 0xCCCCCCCC, 0xF0F0F0F0, 0xFF00FF00, 0xFFFF0000};
  unsigned r = (v & b[0]) != 0;
  #pragma GCC ivdep
  for (short i = 4; i > 0; i--) r|= ((v & b[i]) != 0) << i;
  return r;
}*/
__attribute__((const)) inline unsigned log2_32(unsigned v) {
  static const int MultiplyDeBruijnBitPosition[32] = {
        0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30,
        8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31
  };
  v |= v >> 1; // first round down to one less than a power of 2 
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  return MultiplyDeBruijnBitPosition[(unsigned)(v * 0x07C4ACDDU) >> 27];
}

template<unsigned dimensions>
class Particle{
public:
  Particle() { reset(); }
  inline void reset(){
    for(unsigned i=0; i<dimensions; i++) position_[i]= 0;
  }
  inline void setWorld(unsigned* world) { world_= world; }
  inline void move(){ search(); }
  //inline void move(){ randomWalk(); }

  int position_[dimensions];
private:
  inline void search(){
    //Increase current position -> shift left
    //Read nearby position values
    //Exclude out-of-bounds -> if out-of-bounds, address:= -1
    //Pick rand
    //Select biasing on world vals 
    
    // Copy position locally
    int localPos[dimensions];
    for(unsigned i=0; i<dimensions; i++) localPos[i]= position_[i];

    // Increase current world position
    address_= Address<dimensions-1>::val(localPos);
    if (!(world_[address_] & 0x40000000)){
      if(world_[address_] == 0) world_[address_]++;
      else world_[address_]<<= 1;  //Select bit#30
    }
    
    /*DEBUG*//*cout << "[world]: @"<<address_<<"="<<world_[address_]<<'\t';
    cout << "[pos]: ";
    for(int i=0; i<dimensions; i++) cout<<position_[i]<<',';
    cout <<'\n';*/
    
    // Calc addresses of nearby positions
    for(unsigned i=0; i<dimensions; i++){
      if (localPos[i] < WORLD_SHIFT){
        //cout << "\tp < WS \t\t"<<i<<'\n';
        localPos[i]++;
        nearbyAddress_[i]= Address<dimensions-1>::val(localPos);
        if (localPos[i] > -WORLD_SHIFT+1){
          //cout << "\t-WS <p< WS \t"<<i<<" localPos: "<<localPos[i]<<"\n";
          localPos[i]-=2;
          nearbyAddress_[i+dimensions]= Address<dimensions-1>::val(localPos);
          localPos[i]++;
        } else {
          //cout << "\tp == -WS \t"<<i<<"\n";
          localPos[i]--;
          nearbyAddress_[i+dimensions]= -1;
        }
      } else {
        //cout << "\tp == WS \t"<<i<<"\n";
        nearbyAddress_[i]= -1;
        localPos[i]--;
        nearbyAddress_[i+dimensions]= Address<dimensions-1>::val(localPos);
        localPos[i]++;
      }
    }
    
    // Pick random (weighted) position
    short randS= log2_32((unsigned)rand());   // 30..0 exponentially distributed
    short localWeights[2*dimensions], maxWeight=0, minWeight=30; 
    short maxLessRand=0, maxLessRandI[2*dimensions], j=0;
    for(short i=0; i<2*dimensions; i++){
      if (nearbyAddress_[i] != -1){
        localWeights[i]= 30-log2_32(world_[nearbyAddress_[i]]);
        maxWeight= (localWeights[i] > maxWeight)? localWeights[i]: maxWeight;
        minWeight= (localWeights[i] < minWeight)? localWeights[i]: minWeight;
      } else localWeights[i]= -1;
    }
    //normalize weights so that maximum becomes 30
    if (maxWeight < 30){
      short normalizer= 30-maxWeight;
      for(unsigned i=0; i<2*dimensions; i++) if(localWeights[i] != -1) localWeights[i]+= normalizer;
      minWeight+= normalizer;
    }
    j=0;
    if (minWeight <= randS) {
      for(short i=0; i<2*dimensions; i++)
        if (localWeights[i] == maxLessRand && localWeights[i] != -1) maxLessRandI[j]= i, j++;
        else if (localWeights[i] > maxLessRand && localWeights[i] <= randS && localWeights[i] != -1)
               maxLessRand= localWeights[i], j=0, maxLessRandI[j]= i, j++;
    } else {    // In that case, simply pick everything equal to minWeight
      for(short i=0; i<2*dimensions; i++){
        if (localWeights[i] == minWeight && localWeights[i] != -1) maxLessRandI[j]= i, j++;
      }
    }
    j= randS%j;
    
    //Picked nearbyAddress_[maxLessRandI[j]]
    j= maxLessRandI[j];
    address_= nearbyAddress_[j];
    if (j < dimensions) position_[j]++;
    else position_[j-dimensions]--;

    /*cout << "[new pos]: ";
    for(int i=0; i<dimensions; i++) cout<<position_[i]<<',';
    cout << "\tj="<<j<<'\n';*/

  }
  inline void randomWalk(){
    const unsigned i= rand()%(2*dimensions);
    if(i<dimensions) position_[i]++;
    else position_[i-dimensions]--;
  }
  int address_, nearbyAddress_[2*dimensions];
  unsigned* world_;
};

template<unsigned num_particles= 1, unsigned dimensions= 2, unsigned target_distance= 2>
class Simulation {
public:
  Simulation(ostream& s): outs_(s){
    static_assert(dimensions<=15, "ERROR: Too many dimensions");
    static_assert(dimensions>0, "Dimensions <= 0");
    static_assert(target_distance<=WORLD_SHIFT, "target out-of-bounds");
    static_assert(num_particles > 0, "nmu_particles <= 0");
    for(unsigned i=0; i<num_particles; i++) particles_[i].setWorld(world_);
    for(unsigned i=0; i<dimensions; i++) targetPos_[i]= 0;
    targetPos_[0]= target_distance;
  }
  void run_for(int experimentRuns){
    unsigned time;
    //outs_<<"Experiments number: "<<experimentRuns<<'\n';
    for(int i=0; i<experimentRuns; i++){
      resetWorld();
      time= simulate();
      outs_<<time<<'\n'; 
    }
  }
private:
  void resetWorld(){
    for(unsigned i=0; i<Power<WORLD_BOUNDS,dimensions>::val; i++) world_[i]=0;
  }
  unsigned simulate();
  Particle<dimensions> particles_[num_particles];
  int targetPos_[dimensions];
  unsigned world_[Power<WORLD_BOUNDS,dimensions>::val];
  ostream& outs_;
};

template<unsigned num_particles, unsigned dimensions, unsigned target_distance>
unsigned Simulation<num_particles,dimensions,target_distance>::simulate(){
  unsigned t=0,p,i;
  bool captured= false;
  for(p=0; p<num_particles; p++) particles_[p].reset();
  for(t=0; !captured; t++){
    for(p=0; p<num_particles; p++){
      /*cout << "-> ";
      for(i=0; i<dimensions; i++) cout << particles_[p].position_[i] <<' ';
      cout <<", "<<captured<< '\n';*/

      particles_[p].move();
      for(i=0; i<dimensions; i++) if(particles_[p].position_[i] != targetPos_[i]) break;
      if(i == dimensions){
        captured= true;
        break;
      }
    }
  }
  return t;
}

int main(int argc, char **argv){
  static_assert(sizeof(unsigned) == 4, "Bithacks can't work on this system");
  //srand(time(0));
  srand(1);
  Simulation<NUM_PARTICLES,DIMENSIONS,TARGET_DISTANCE> sim(cout);
  unsigned times=1000;
  if(argc == 2) times= strtol(argv[1], NULL, 10);
  sim.run_for(times);
  return 0;
}
