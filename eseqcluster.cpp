#include "eseqcluster.h"
#include "cluster-common.h"

#include <eutils/efile.h>

eseqdist::eseqdist() {}
eseqdist::eseqdist(INDTYPE _x,INDTYPE _y,float _dist): x(_x),y(_y),dist(_dist) {}

void eseqdist::serial(estr& data) const
{
  ::serial(x,data); ::serial(y,data); ::serial(dist,data);
//  ::serial(x,data); ::serial(y,data); ::serial(dist,data); ::serial(count,data);
}

int eseqdist::unserial(const estr& data,int i)
{
  i=::unserial(x,data,i);
  if (i==-1) return(i);
  i=::unserial(y,data,i);
  if (i==-1) return(i);
  i=::unserial(dist,data,i);
//  if (i==-1) return(i);
//  i=::unserial(count,data,i);
  return(i);
}



//bool eseqcount::operator==(const eseqcount& scount) const{ return(x==scount.x&&y==scount.y || x==scount.y&&y==scount.x); }

//unsigned int seqcount_hash(const eseqcount& scount){ return(scount.x*scount.y); }

//eseqcluster::eseqcluster(): smatrix(seqcount_hash) {}
eseqcluster::eseqcluster(){}


void eseqcluster::check(ebasicarray<eseqdist>& dists)
{
  long i;
  estr xystr;
//  eseqcount xy;
  bool duplicate=false;
  for (i=0; i<dists.size(); ++i){
    if (i%(dists.size()/10)==0) { cout << i*10/dists.size(); flush(cout); }
    xy2estr(dists[i].x,dists[i].y,xystr);

    ebasicstrhashof<INDTYPE>::iter it;

//    cout << dists[i].dist << " " << dists[i].x << " " << dists[i].y;
    it=smatrix.get(xystr);
    if (it != smatrix.end()) {
      cout << "duplicate found: "+estr(dists[i].x)+","+dists[i].y << endl;
//      cout << " *";
      duplicate=true;
    }else
      smatrix.add(xystr,1);
//    cout << endl;
  }
  smatrix.clear();

  ldieif(duplicate,"duplicates found");
  cout << "# no duplicates found!" << endl;
}

void eseqcluster::init(INDTYPE count,const estr& ofilename,const estr& seqsfile,const earray<ebasicarray<INDTYPE> >& dupslist)
{
  ofile.open(ofilename,"w");
  ofile.write("# seqsfile: "+seqsfile+"\n");
  ofile.write("# OTU_count Merge_distance Merged_OTU_id1 Merged_OTU_id2\n");

  long i,j;

  mergecount=0;
  scount.reserve(count);
  scluster.reserve(count);
  smerge.reserve(count);
  inter.reserve(count);
  incluster.reserve(count);
  for (i=0; i<count; ++i){
    scount.add(1);
    scluster.add(i);
    smerge.add(-1);
    incluster.add(list<INDTYPE>());
    incluster[i].push_back(i);
    inter.add(list<INDTYPE>());
  }
  for (i=0; i<dupslist.size(); ++i){
    for (j=1; j<dupslist[i].size(); ++j){
      ++mergecount;
      ofile.write(estr(scluster.size()-mergecount)+" 1.0 "+dupslist[i][0]+" "+dupslist[i][j]+"\n");
      clusterData.mergearr.add(eseqdist(dupslist[i][0],dupslist[i][j],1.0));
    }
  }
  cout << "# initializing cluster with: "<< count<< " seqs" << endl; 
  INDTYPE ucount=dupslist.size()*dupslist.size()/20000l/2l;
  cout << "# initializing smatrix with: " << ucount << " elements" << endl; 
  smatrix.reserve(ucount);
//  cout << "# smatrix._hashitems = " << smatrix._hashitems << endl;
}

void eseqcluster::merge(INDTYPE x,INDTYPE y,float dist)
{
  ldieif(x==y,"should not happen!");
  ldieif(scount[x]==0 || scount[y]==0,"also should not happen");

  clusterData.mergearr.add(eseqdist(x,y,dist));

  smerge[x]=x;
  smerge[y]=x;

  scount[x]+=scount[y];
  scount[y]=0;

  list<INDTYPE>::iterator it;
  for (it=incluster[y].begin(); it!=incluster[y].end(); ++it){
    scluster[*it]=x;
    incluster[x].push_back(*it);
  }

  estr tmpstr,tmpstr2;

  INDTYPE i,j;
  for (it=inter[y].begin(); it!=inter[y].end(); ++it){
    j=scluster[*it];
    if (x==j || y==j) continue;
    xy2estr(x,j,tmpstr);
    xy2estr(y,j,tmpstr2);

    ebasicstrhashof<INDTYPE>::iter tmpit2=smatrix.get(tmpstr2);
    if(tmpit2==smatrix.end()) continue;

    ebasicstrhashof<INDTYPE>::iter tmpit=smatrix.get(tmpstr);
    if (tmpit!=smatrix.end())
      *tmpit+=*tmpit2;
    else{
      smatrix.add(tmpstr,*tmpit2);
      inter[x].push_back(j);
    }
    smatrix.erase(tmpit2);
  }
  ++mergecount;
}

void eseqcluster::add(const eseqdist& sdist){
//  if (sdist.count==0) return;
  ldieif(sdist.x<0 || sdist.y<0 || sdist.x>=scluster.size() || sdist.y>=scluster.size(),"out of bounds: sdist.x: "+estr(sdist.x)+" sdist.y: "+estr(sdist.y)+" scluster.size(): "+estr(scluster.size()));

  INDTYPE x=scluster[sdist.x];
  INDTYPE y=scluster[sdist.y];

  ldieif(x<0 || y<0 || x>=scluster.size() || y>=scluster.size(),"out of bounds: sdist.x: "+estr(x)+" sdist.y: "+estr(y)+" scluster.size(): "+estr(scluster.size()));
  INDTYPE tmp;
  if (x>y) { tmp=x; x=y; y=tmp; }

  INDTYPE links;
  estr xystr;

//  cout << x << " " << y << " " << sdist.dist << endl;
  ldieif(x==y,"should not happen: "+estr(x)+","+estr(y)+" --- "+estr(sdist.x)+","+estr(sdist.y));

  xy2estr(x,y,xystr);

  ebasicstrhashof<INDTYPE>::iter it;

  it=smatrix.get(xystr);
  if (it==smatrix.end()){
//    if (scount[x]*scount[y]==sdist.count){
    if (scount[x]*scount[y]==1){
      merge(x,y,sdist.dist);
//      cout << scluster.size()-mergecount << " " << sdist.dist << " " << x << " " << y << endl;
      ofile.write(estr(scluster.size()-mergecount)+" "+sdist.dist+" "+x+" "+y+"\n");
    }else{
//      smatrix.add(xystr,sdist.count);
      smatrix.add(xystr,1);
      inter[x].push_back(y); inter[y].push_back(x);
    }
    return;
  }

//  *it+=sdist.count;
  ++(*it);

  // complete linkage
  if (*it==scount[x]*scount[y]){
    merge(x,y,sdist.dist);
//    cout << scluster.size()-mergecount << " " << sdist.dist << " " << x << " " << y << endl;
    ofile.write(estr(scluster.size()-mergecount)+" "+sdist.dist+" "+x+" "+y+"\n");
    smatrix.erase(it);
  }
}

/*
void eseqcluster::add(int ind){
//  if (dists[ind].count==0) return;
  ldieif(dists[ind].x<0 || dists[ind].y<0 || dists[ind].x>=scluster.size() || dists[ind].y>=scluster.size(),"out of bounds: dists[ind].x: "+estr(dists[ind].x)+" dists[ind].y: "+estr(dists[ind].y)+" scluster.size(): "+estr(scluster.size()));

  int x=scluster[dists[ind].x];
  int y=scluster[dists[ind].y];

  ldieif(x<0 || y<0 || x>=scluster.size() || y>=scluster.size(),"out of bounds: dists[ind].x: "+estr(x)+" dists[ind].y: "+estr(y)+" scluster.size(): "+estr(scluster.size()));
  int tmp;
  if (x>y) { tmp=x; x=y; y=tmp; tmp=dists[ind].x; dists[ind].x=dists[ind].y; dists[ind].y=tmp; }

  int links;
  int i;
  estr xystr;

//  cout << x << " " << y << " " << dists[ind].dist << endl;
  ldieif(x==y,"should not happen: "+estr(x)+","+estr(y)+" --- "+estr(dists[ind].x)+","+estr(dists[ind].y));

  xy2estr(x,y,xystr);

  ebasicstrhashof<int>::iter it;

  it=smatrix.get(xystr);
  if (it==smatrix.end()){
//    if (scount[x]*scount[y]==dists[ind].count){
    if (scount[x]*scount[y]==1){
      merge(x,y);
      cout << scluster.size()-mergecount << " " << dists[ind].dist << " " << x << " " << y << endl;
    }else{
//      smatrix.add(xystr,dists[ind].count);
      smatrix.add(xystr,1);
      inter[x].push_back(y); inter[y].push_back(x);
    }
    return;
  }

//  *it+=dists[ind].count;
  ++(*it);

  // complete linkage
  if (*it==scount[x]*scount[y]){
    merge(x,y);
    cout << scluster.size()-mergecount << " " << dists[ind].dist << " " << x << " " << y << endl;
//    cout << dists[ind].dist << " " << x << " " << y << endl;
    smatrix.erase(it);
  }
}
*/

void eseqcluster::save(const estr& filename,const estrarray& arr)
{
  long i;
  estr otustr;
  estrhashof<ebasicarray<INDTYPE> > otus;

  efile f(filename+".clstr");
  for (i=0; i<scluster.size(); ++i)
    f.write(estr(scluster[i])+"     "+arr.keys(i)+"\n");
  f.close();

  list<INDTYPE>::iterator it;
  long otucount=0;
  efile f2(filename);
  for (i=0; i<incluster.size(); ++i){
    if (scount[i]==0) continue;
    f2.write(">OTU"+estr(otucount)+"\n");
    for (it=incluster[i].begin(); it!=incluster[i].end(); ++it)
      f2.write(arr.keys(*it)+"\n");
    ++otucount;
  }
  f2.close();
}

