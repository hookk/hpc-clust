#include "eparserinterpreter.h"

#include "eparser.h"
#include "evarcommon.h"
#include "logger.h"
#include "evar.h"
#include "eiostream.h"

#ifdef EUTILS_HAVE_READLINE_H
 #include <readline/readline.h>
 #include <readline/history.h>
#endif

class eatom_base;

bool isOp(const estr& str);
bool split_atoms2(const estr& str,estrarray& sa);

typedef void (*epop_f)(evar&,estrhashof<evar>&,const estr& name,ebasicarray<eatom_base*>&,evararray&);

void assign(evar&,estrhashof<evar>&,const estr& name,ebasicarray<eatom_base*>&,evararray&);
void assignref(evar&,estrhashof<evar>&,const estr& name,ebasicarray<eatom_base*>&,evararray&);
void objprop(evar&,estrhashof<evar>&,const estr& name,ebasicarray<eatom_base*>& args,evararray&);
void objop(evar&,estrhashof<evar>&,const estr& name,ebasicarray<eatom_base*>& args,evararray&);
void objcall(evar&,estrhashof<evar>&,const estr& name,ebasicarray<eatom_base*>& args,evararray&);

class eatom_base
{
 public:
  int type;
  bool evald;
  estr exechost;
  int remote;

//  bool internal;
  evar rvalue;
  ebasicarray<eatom_base*> args;
  eatom_base(int type);
  virtual ~eatom_base();
  virtual evar& make(estrhashof<evar>& env)=0;
  virtual void clear()=0;
  virtual void print(estr& s)=0;
  virtual void serial(estr& data) const=0;
  virtual int unserial(const estr& data,int pi)=0;
};

eatom_base::eatom_base(int _type): type(_type), evald(false), remote(0) {}
eatom_base::~eatom_base()
{
  int i;
  for (i=0; i<args.size(); ++i)
    delete args[i];
  args.clear();
}

class eatom_value : public eatom_base
{
 public:
  estr value;
  evar& make(estrhashof<evar>& env);
  void print(estr& s);
  void clear();

  eatom_value(): eatom_base(2) {}
  eatom_value(const estr& str);

  void serial(estr& data) const;
  int unserial(const estr& data,int ip);
};


void eatom_value::serial(estr& data) const
{
  value.serial(data);
  rvalue.serial(data);
}

int eatom_value::unserial(const estr& data,int ip)
{
  ip=value.unserial(data,ip);
  ip=rvalue.unserial(data,ip);
  return(ip);
}



void eatom_value::clear()
{
  rvalue.clear();
  evald=false;
}

eatom_value::eatom_value(const estr& str): eatom_base(2),value(str)
{
/*
  ldinfo("eatom_value::make()");
  if (evald && rvalue.isNull() && env.exists(value)) // environment variables are not stored in rvalue
    return(env[value]);
  if (!rvalue.isNull() || evald) return(rvalue);

  evald=true;

  if (!value.len()){
    lderror("empty atom value");
    return(rvalue);
  }

  if (value.is_int()){
    rvalue.set(value.i());
    return(rvalue);
  }

  if (value.is_float()){
    rvalue.set(value.f());
    return(rvalue);
  }

  if (value.is_hex()){
    rvalue.set(value.h());
    return(rvalue);
  }
  if (value[0]=='@'){
    int i;
    i=value.find(":");
    if (i==-1){
      lderror("remote variable missing :");
      return(rvalue);
    }
    estr tmphost(value.substr(1,i-1));
    estr tmpvar(value.substr(i+1));
    exechost=tmphost;
    rvalue.set(getDistComp().var(tmphost,tmpvar));
    return(rvalue);
  }

  if (value[0]=='"' && value[value.len()-1]=='"'){
    estr tmps;
    tmps=value.substr(1,value.len()-2);
    tmps.replace("\\\"","\"");
    tmps.replace("\\n","\n");
    tmps.replace("\\r","\r");
    tmps.replace("\\t","\t");
    tmps.replace("\\\\","\\");
    rvalue.set(new estr(tmps));
    return(rvalue);
  }

  if (getClassNames().exists(value)){
    rvalue.set(getClassNames()[value]->create.at(0));
    return(rvalue);
  }

  if (env.exists(value))
    return(env[value]);

  if (parser->funcs.exists(value)){
    rvalue.set(parser->funcs[value].at(0));
    return(rvalue);
  }

  if (&env==&getParser()->objects) lwarn("creating variable \""+value+"\"");
  env.add(value,evar());
  return(env[value]);
*/
}

evar& eatom_value::make(estrhashof<evar>& env)
{
  ldinfo("eatom_value::make()");
  if (evald && rvalue.isNull() && env.exists(value)) // environment variables are not stored in rvalue
    return(env[value]);
  if (!rvalue.isNull() || evald) return(rvalue);

  evald=true;

  if (!value.len()){
    lderror("empty atom value");
    return(rvalue);
  }

  if (value.is_int()){
    rvalue.set(value.i());
    return(rvalue);
  }

  if (value.is_float()){
    rvalue.set(value.f());
    return(rvalue);
  }

  if (value.is_hex()){
    rvalue.set(value.h());
    return(rvalue);
  }

  if (value[0]=='"' && value[value.len()-1]=='"'){
    estr tmps;
    tmps=value.substr(1,value.len()-2);
    tmps.replace("\\\"","\"");
    tmps.replace("\\n","\n");
    tmps.replace("\\r","\r");
    tmps.replace("\\t","\t");
    tmps.replace("\\\\","\\");
    rvalue.set(new estr(tmps));
    return(rvalue);
  }

  if (getClassNames().exists(value)){
    rvalue.set(getClassNames()[value]->create.at(0));
    return(rvalue);
  }

  if (env.exists(value))
    return(env[value]);

  if (parser->funcs.exists(value)){
    rvalue.set(parser->funcs[value].at(0));
    return(rvalue);
  }

  if (&env==&getParser()->objects) lwarn("creating variable \""+value+"\"");
  env.add(value,evar());
  return(env[value]);
}

void eatom_value::print(estr& s)
{
  ldinfo("eatom_value::print");
  if (!value.len()){
    lderror("empty atom value");
    s += "(empty) ";
    return;
  }

  if (value.is_int()){
    s += "("+value+") ";
    return;
  }

  if (value.is_float()){
    s += "("+value+") ";
    return;
  }

  if (value.is_hex()){
    s += "("+value+") ";
    return;
  }

  if (value[0]=='"' && value[value.len()-1]=='"'){
    s += "\""+value.substr(1,value.len()-2)+"\" ";
    return;
  }


  if (parser->objects.exists(value)){
    s += value + " "; //+ "(" + *parser->objects[value]<<") ";
    return;
  }

  s += "<n/a>"+value+" ";
  return;
}

class eatom : public eatom_base
{
 public:
  estr name;
  evararray vargs;
  epop_f fcall;
  efunc *efcall;
  evar& make(estrhashof<evar>& env);
  void print(estr& s);
  void clear();

  eatom(): eatom_base(1) {}
  eatom(const estrarray& strarr);

  void serial(estr& data) const;
  int unserial(const estr& data,int i);
};

void eatom::serial(estr& data) const
{
  name.serial(data);
  serialint(args.size(),data);
  int i;
  for (i=0; i<args.size(); ++i){
    serialint(args[i]->type,data);
    args[i]->serial(data);
  }
}

int eatom::unserial(const estr& data,int ip)
{
  ip=name.unserial(data,ip);
  unsigned int count,type;
  int i;
  ip=unserialint(count,data,ip);
  if (ip==-1) return(ip);
  for (i=0; i<count; ++i){
    ip=unserialint(type,data,ip);
    if (ip==-1) return(ip);
    if (type==1){
      eatom *tmpatom=new eatom;
      ip=tmpatom->unserial(data,ip);
      args.add(tmpatom);
    } else if (type==2){
      eatom_value *tmpatom=new eatom_value;
      ip=tmpatom->unserial(data,ip);
      args.add(tmpatom);
    }
    if (ip==-1) return(ip);
  }
  return(ip);
}

efunc* findFunc(earray<efunc>& farr,const evararray& arr)
{
  int i,j;
  ldieif(farr.size()==0,"looking for overloaded function in empty function array!");

  if (farr.size()==1)
    return(&farr[0]);

  efunc *f;
  int match;

  // TODO: Improve overloaded function matching. we can check if conversion is possible
  for (i=0; i<farr.size(); ++i){
    f=&farr[i];
    match=1;
    
    for (j=0; j<arr.size() && j<f->fArgs.size(); ++j){
      if (arr[j].getTypeid() != *f->fArgs[j]){
        match=0;
        break;
      }
    }
//    cout << "match: "<<match<<" arr: "<<arr.size()<<" fArgs: "<<f->fArgs.size()<<" defargs: "<<f->defargs.size()<<endl;
    if (match==1 && arr.size() == f->fArgs.size())
      return(&farr[i]);
    else if (match==1 && arr.size() <= f->fArgs.size() && arr.size() + f->defargs.size() >= f->fArgs.size()){
      lwarn("overloaded function with ambiguous argument set, using arbitrary function");
      cout << farr[i] << endl;
      return(&farr[i]);
    }
  }
  lwarn("overloaded function with ambiguous argument set, using first function");
  return(&farr[0]);
}


void eatom::clear()
{
  int i;
  for (i=0; i<args.size(); ++i)
    args[i]->clear();
  rvalue.clear();
//  vargs.clear();
  evald=false;
}

evar& eatom::make(estrhashof<evar>& env)
{
  ldinfo("eatom::make, name: "+name);
  if (!rvalue.isNull() || evald)
    return(rvalue);

  evald=true;

/*
  // optimize calls in reused code such as loop conditionals and loop code
  if (fcall){
    fcall(rvalue,env,name,args,vargs);
    return(rvalue);
  }else if (efcall){
    int i;
    for (i=1; i<args.size(); ++i)
      vargs[i-1].set(args[i]->make(env));
    rvalue.set(efcall->call(vargs));
    return(rvalue);
  }
*/

  if (name.len()==0 && args.size()==1){
    rvalue.set(args[0]->make(env));
    return(rvalue);
  }
  lderrorif(name.len()==0,"no name in eatom, args.size="+estr(args.size()));

  if (name=="="){
    fcall=&assign;
    assign(rvalue,env,name,args,vargs);
    return(rvalue);
  }else if (name=="=&"){
    fcall=&assignref;
    assignref(rvalue,env,name,args,vargs);
    return(rvalue);
  }else if (name=="."){
    fcall=&objprop;
    objprop(rvalue,env,name,args,vargs);
    return(rvalue);
  }else if (name=="()"){
    estr fname;
    if (args[0]->type==2){
      fname=static_cast<eatom_value*>(args[0])->value;
      if (getClassNames().exists(fname)){
        int i;
        for (i=1; i<args.size(); ++i)
          vargs[i-1].set(args[i]->make(env));
//          vargs.add(args[i]->make(env));
        efcall=findFunc(getClassNames()[fname]->create,vargs);
        efcall->call(rvalue,vargs);
        return(rvalue);
      }else if (parser->funcs.exists(fname)){
        int i;
        for (i=1; i<args.size(); ++i)
          vargs[i-1].set(args[i]->make(env));
//          vargs.add(args[i]->make(env));
        efcall=findFunc(parser->funcs[fname],vargs);
        efcall->call(rvalue,vargs);
        return(rvalue);
      }else if (env.exists(fname)){
        fcall=objop;
        objop(rvalue,env,name,args,vargs);
        return(rvalue);
      }else
        lerror("unknown function or object: "+fname);
    } else if (args[0]->type==1){
      if (static_cast<eatom*>(args[0])->name=="."){
        fcall=objcall;
        objcall(rvalue,env,name,args,vargs);
        return(rvalue);
      }else if (!args[0]->make(env).isNull()) {
        fcall=objop;
        objop(rvalue,env,name,args,vargs);
        return(rvalue);
      } else
        lerror("left side of operator() does not resolve to an object");
    }
    return(rvalue);
  }else{  //if (isOp(name) || name=="[]" || name=="++prefix" || name=="--prefix"){
    fcall=&objop;
    objop(rvalue,env,name,args,vargs);
    return(rvalue);
  }

  lwarn("unknown operator: "+name);
  return(rvalue);
}

void eatom::print(estr& s)
{
  ldinfo("eatom::print "+name+" args.size: "+estr(args.size()));
  s += name+" { ";

  int i;
  for (i=0; i<args.size(); ++i)
    { args[i]->print(s); s += ", "; }
  if (args.size()>0)
    s.del(-2);

  s += "} ";
}


eatom_base* newFuncAtom(const estr& name,const estrarray& sa)
{
  estrarray tmpsa;
  split_atoms2(sa[0],tmpsa);
  eatom* atom=new eatom(tmpsa);
  atom->name = name;
  return(atom);
}


eatom_base* newAtom(const estrarray& sa)
{
  if (!sa.size()) return(new eatom(sa));

  if (sa.size()>1)
    return(new eatom(sa));

  estrarray tmpsa;
  if (sa[0].len() && sa[0][0]=='(')
    split_atoms2(sa[0].substr(1,-2).trim(),tmpsa);
  else
    split_atoms2(sa[0],tmpsa);
  if (tmpsa.size()==1 && tmpsa[0].len() && tmpsa[0][0]=='['){
    tmpsa[0]=tmpsa[0].substr(1,-2);
    tmpsa.insert(0,"evararray");
    tmpsa.insert(1,"(");
    tmpsa.add(")");
    return(new eatom(tmpsa));
  }else if (tmpsa.size()==1)
    return(new eatom_value(tmpsa[0]));

  return(new eatom(tmpsa));
}

int find_min_assoc_op(const estrarray& sa);

eatom::eatom(const estrarray& sa): eatom_base(1),fcall(0x00),efcall(0x00)
{
  if (!sa.size()){ ldinfo("eatom(): empty array"); return; }

  int i;
  if (sa[0][0]=='$'){
    // external executable call
    name="()";
    args.add(newAtom("exec"));
    args.add(newAtom("\""+sa[0].substr(1)+"\""));
    for (i=1; i<sa.size(); ++i)
      args.add(newAtom("\""+sa[i]+"\""));
    vargs.init(args.size()-1);
    return;
  }

  if (sa.size()==1){
    if (!sa[0].len())
      return;
    ldinfo("eatom(): 1 element: "+estr(sa));
    args.add(newAtom(sa));
    return;
  }

  i=find_min_assoc_op(sa);
  if (i==-1){
    lerror("eatom(): unprocessed event: "+estr(sa));
    return;
  }
   
  if (sa[i][0]=='('){
    ldinfo("eatom(): operator found: "+sa[i]+" in: "+estr(sa));
    name="()";
    args.add(newAtom(sa.subset(0,i)));
    estrarray tmpsa;
    split_atoms2(sa[i].substr(1,-2).trim(),tmpsa);

    int i,e;
    for (i=0,e=tmpsa.find(","); e!=-1; i=e+1,e=tmpsa.find(",",i)){
      args.add(newAtom(tmpsa.subset(i,e-i)));
    }
    if (tmpsa.size() && tmpsa[i].len())
      args.add(newAtom(tmpsa.subset(i)));
/*
    e=tmpsa.find(",");
    if (e!=-1){
      i=0;
      while (e!=-1) {
        args.add(newAtom(tmpsa.subset(i,e-i)));
        i=e+1;
        e=tmpsa.find(",",i);
      }
      args.add(newAtom(tmpsa.subset(i)));
    }else if (tmpsa.size() && tmpsa[0].len())
      args.add(newAtom(tmpsa));
*/
  } else if (sa[i][0]=='['){
    ldinfo("eatom(): operator found: "+sa[i]+" in: "+estr(sa));
    name="[]";
    args.add(newAtom(sa.subset(0,i)));

    estrarray tmpsa;
    split_atoms2(sa[i].substr(1,-2).trim(),tmpsa);
    args.add(newAtom(tmpsa));
  }else{
    ldinfo("eatom(): operator found: "+sa[i]+" in: "+estr(sa));
    name=sa[i];
    if (name=="++"){
      if (i==0) {
        name="++prefix";
        args.add(newAtom(sa.subset(i+1)));
      } else
        args.add(newAtom(sa.subset(0,i)));
    }else if (name=="--"){
      if (i==0) {
        name="--prefix";
        args.add(newAtom(sa.subset(i+1)));
      } else
        args.add(newAtom(sa.subset(0,i)));
    }else if (i==0 && (name=="/" || name==".")){
      if (sa.size()>1){
        estrarray tmpsa(sa);
        tmpsa[1]=tmpsa[0]+tmpsa[1];
        args.add(newAtom(tmpsa.subset(1)));
      }else{
        args.add(newAtom(sa));
      }
    }else if (name=="&" && i>0 && sa[i-1]=="="){
      name="=&";
      args.add(newAtom(sa.subset(0,i-1)));
      args.add(newAtom(sa.subset(i+1)));
    }else{
      args.add(newAtom(sa.subset(0,i)));
      args.add(newAtom(sa.subset(i+1)));
    }
  }
  vargs.init(args.size()-1);
}

estrarray ops=estr(".:++:--:*:%:/:-:+:>>:<<:,:=:+=:-=:==:!=:>=:<=:>:<:&:^:|:&&:||: ").explode(":");
//estrarray ops=estr(".:++:--:*:%:/:-:+:>>:<<:,:=:+=:-=:==:!=:>=:<=:>:<:&:^:|:&&:||").explode(":");
//estrarray ops=estr("-=:+=:=:.:,:*:%:/:-:+:<<:>>:<:>:<=:>=:!=:==").explode(":");
//==:!=:>=:<=:>:<:>>:<<:+:-:/:%:*:,:.").explode(":");

int find_min_assoc_op(const estrarray& sa)
{
  long i,j;
  long imin,ipos;
  imin=-1;
  ipos=-1;
  for (i=0; i<sa.size(); ++i){
    j=ops.find(sa[i]);
    if (sa[i][0]=='(') j=0;
    else if (sa[i]==".") j=0;
    else if (sa[i][0]=='[') j=0;
    else if (j==-1) continue;

//    if (j>imin && ipos!=i || ((j==0 || j==5) && imin==0)){ imin=j; ipos=i; } // we look for the operator that should be evaluated last, in the case of the "." it is the last "." found in the string the "/" is also in the same case
    if (j>imin || (j==0 && imin==0) || ((j==5 || j==3) && (imin==5 || imin==3))){ imin=j; ipos=i; } // we look for the operator that should be evaluated last, in the case of the "." it is the last "." found in the string the "/" is also in the same case
  }
//  cout << "div: "<<ipos<<" op: "<<ops[
  return(ipos);
}


long find_ops(const estr& str,const estr& op,long i=0)
{
  long j;
  bool start;
  start=true;
  for (; i<str.len(); ++i){
    if (start && str[i]>='0' && str[i]<='9' || str[i]=='-' && i+1<str.len() && str[i+1]>='0' && str[i+1]<='9'){
      ++i;
      for (;i<str.len() && (str[i]>='0' && str[i]<='9' || str[i]=='.' || str[i]=='e' || str[i]=='E' || str[i]=='x' || str[i]=='-' && (str[i-1]=='e' || str[i-1]=='E') ); ++i);
      start=false;
    }else if (start && str[i]=='"'){
      ++i;
      for (;i<str.len() && str[i]!='"'; ++i){ if (str[i]=='\\') ++i; }
      start=false;
    }else
      start=false;

    if (!start && str[i]==op[0]){
      j=0;
      for (;i+j<str.len() && j<op.len() && str[i+j]==op[j]; ++j);
      if (j==op.len()) return(i);
    }
  }
  return(-1);
}

void skip_blank(const estr& str,long& i){ for (;i<str.len() && str[i]==' '; ++i); }

void split_ops(const estr& str, estrarray& strarr, const estrarray& ops)
{
  long i2;
  long io;
  long iopos;
  long i;
//  int e;

  long j;

  i=0;
  while(i<str.len()){
    skip_blank(str,i);
    io=0; iopos=find_ops(str,ops[0],i);
    if (iopos==-1) iopos=str.len();
    for (j=0;j<ops.size(); ++j){
      i2=find_ops(str,ops[j],i);
      if (i2 < iopos && i2!=-1) { iopos=i2; io=j; }
      else if (i2 != -1 && i2 == iopos && ops[j].size() == 2){ io=j; }
    }
    if (iopos==str.len() ) break;

    if (iopos-i)
      strarr += str.substr(i,iopos-i).trim();
    if (ops[io]!=" ") // spaces should only split arguments not be added as operators
      strarr += ops[io];
    i = iopos + ops[io].len();
  }

  if (i!=str.len())
    strarr += str.substr(i).trim();
}




bool isOp(const estr& str)
{
  if (ops.find(str)!=-1) return(true);
  return(false);
}

inline bool streq(const estr& str,const estr& m,int i)
{
  if (str.len()-i<m.len()) return(false);

  int j;
  for (j=0; j<m.len(); ++j)
    if (str[j+i]!=m[j]) return(false);

  return(true);
}

bool find_blocks(const estr& str,const estr& blockleft,const estr& blockright,long& i,long& e)
{
  long j;
  long entry;
  for (;i<str.len(); ++i){
    if (str[i]=='"'){
      ++i;
      for (;i<str.len() && str[i]!='"'; ++i){ if (str[i]=='\\') ++i; }
    }else{
      for (j=0; j<blockleft.size(); ++j){
        if (str[i]==blockleft[j]){     // for left,right block endings bigger than 1 char:  && streq(str,blockleft[j],i)){
          entry=1;
          e=i+1;
          for (;e<str.size();++e){
            if (str[i]=='"'){
              ++i;
              for (;i<str.len() && str[i]!='"'; ++i){ if (str[i]=='\\') ++i; }
            }else if (str[e]==blockleft[j])  // && streq(str,blockleft
              ++entry;
            else if (str[e]==blockright[j]){
              --entry;
              if (!entry) return(true);
            }
          }
          if (e==str.size()){
            lwarn("closing string \""+blockright.substr(j,1)+"\" missing in \""+str.substr(i)+"\"");
            i=str.size();
            return(false);
          }
        }
      }
    }
  }
  e=i;
  return(true);
}

bool split_atoms2(const estr& str, estrarray& strarr)
{
  estr left="([{";
  estr right=")]}";
  
  long i,i2,e;

  e=0;
  i=0;
  i2=0;
  bool err;
  err=false;

  while (i2<str.len() && (err=find_blocks(str,left,right,i2,e))){
    ldinfo(str.substr(i,i2-i) + " --- " + str.substr(i2,e-i2) + " --- " + str.substr(e));
    split_ops(str.substr(i,i2-i),strarr,ops);
    if (e==str.len()) {
      i=i2;
      break;
    }
    strarr += str.substr(i2,e-i2+1).trim();
    i=e+1;
    i2=i;
  }
  if (i!=str.len())
    split_ops(str.substr(i),strarr,ops);

  if (!err)
    strarr.clear();

  return(err);
}


void objcall(evar& rvalue,estrhashof<evar>& env,const estr& name,ebasicarray<eatom_base*>& args,evararray& vargs)
{
  int i;
  for (i=1; i<args.size(); ++i){
    vargs[i-1].set(args[i]->make(env));
    if (vargs[i-1].isNull()) { lwarn("argument is null: "+estr(i-1)); return; }
  }
  args[0]->args[0]->make(env).call(rvalue, static_cast<eatom_value*>(args[0]->args[1])->value , vargs);
}

void objop2(evar& rvalue,estrhashof<evar>& env,const estr& name,ebasicarray<eatom_base*>& args,evararray& vargs)
{
  int i;
  for (i=1; i<args.size(); ++i){
    vargs[i-1].set(args[i]->make(env));
    if (vargs[i-1].isNull()) { lwarn("argument is null: "+estr(i-1)); return; }
  }
  ldinfo("objop: "+estr(args.size()));
  args[0]->make(env).call( rvalue, name, vargs);
}

bool isPrimitive(const type_info& tinfo)
{
  if (tinfo==typeid(int) || tinfo==typeid(unsigned int) || tinfo==typeid(char) || tinfo==typeid(unsigned char) || tinfo==typeid(short) || tinfo==typeid(unsigned short) || tinfo==typeid(float) || tinfo==typeid(double) || tinfo==typeid(long) || tinfo==typeid(unsigned long))
    return(true);
  return(false);
}

void objop(evar& rvalue,estrhashof<evar>& env,const estr& name,ebasicarray<eatom_base*>& args,evararray& vargs)
{
  int i;
  if (!isPrimitive(args[0]->make(env).getTypeid())){
    for (i=1; i<args.size(); ++i){
      vargs[i-1].set(args[i]->make(env));
      if (vargs[i-1].isNull()) { lwarn("argument is null: "+estr(i-1)); return; }
    }
    ldinfo("objop: "+estr(args.size()));
    args[0]->make(env).call( rvalue, name, vargs);
    return;
  }

  // primitive operators need to be passed the object as the first argument
  if (vargs.size()<args.size()) vargs.add(evar());
  for (i=0; i<args.size(); ++i){
    vargs[i].set(args[i]->make(env));
    if (vargs[i].isNull()) { lwarn("argument is null: "+estr(i)); return; }
  }
  ldinfo("objop: "+estr(args.size()));
  args[0]->make(env).call( rvalue, name, vargs);
}

void objprop(evar& rvalue,estrhashof<evar>& env,const estr& name,ebasicarray<eatom_base*>& args,evararray& vargs)
{
  evar obj;
  if (args.size()==2){
    if (args[1]->type!=2) { lerror("property next to . is not a string?"); return; }
    obj.set(args[0]->make(env));
    estr prop(static_cast<eatom_value*>(args[1])->value);
   
    if (obj.hasProperty(prop))
      rvalue.set(obj.property(prop));
    else if (obj.hasMethod(prop))
      rvalue.set(obj.property(prop));
//      tmp= args[0].make(env).property( ((eatom_value*)&args[1])->value ).var;
/*
    if (args[1].type==2)
      tmp = args[0].make(env).get( ((eatom_value*)&args[1])->value ).var;
    else if (args[1].args.size() && args[1].args[0].type==2){
      int i;
      evararray arr;
      for (i=1; i<args[1].args.size(); ++i)
        arr.addref(new evar(args[1].args[i].make(env).var));
      tmp = args[0].make(env).call( ((eatom_value*)&args[1].args[0])->value, arr).var;
    }
*/
  }else
    lerror("objprop argument size mismatch: "+estr(args.size()));
}

void assign(evar& rvalue,estrhashof<evar>& env,const estr& name,ebasicarray<eatom_base*>& args,evararray& vargs)
{
  ldinfo("assigning value");
  if (args[0]->make(env).isNull())
    args[0]->make(env).copy(args[1]->make(env));
  else
    args[0]->make(env)=args[1]->make(env);
  rvalue.set(args[0]->make(env));
}

void assignref(evar& rvalue,estrhashof<evar>& env,const estr& name,ebasicarray<eatom_base*>& args,evararray& vargs)
{
  ldinfo("assigning value");
  args[0]->make(env).set(args[1]->make(env));
  rvalue.set(args[0]->make(env));
}



// TODO: Make sure code Atoms are all deleted after parsing!!!






enum catype{CA_CODE,CA_BLOCK,CA_ARG,CA_NUM,CA_STR};

class ecodeAtom
{
 public:
  catype type;
  estr text;

  virtual evar interpret(estrhashof<evar>& env)=0;
  virtual ~ecodeAtom() {}
};


void getcontrolstr(const estr&,int&,estr&); 
ecodeAtom *getcodeifatom(const estr&,int&);
ecodeAtom *getcodeforatom(const estr&,int&);
ecodeAtom *getcodewhileatom(const estr&,int&);
ecodeAtom *getcodedoatom(const estr&,int&);
ecodeAtom *getsingleatom(const estr&,int&);
ecodeAtom *getcodeatom(const estr& str,int& ind);
ecodeAtom *getcodefunctionatom(const estr& str,int& ind);
ecodeAtom *getatom(const estr& str,int& ind);




class ecodeAtomBlock : public ecodeAtom
{
 public:
  ebasicarray<ecodeAtom*> exec;

  ~ecodeAtomBlock();

  void clear();
  virtual evar interpret(estrhashof<evar>& env);
  void parse(const estr& str);
};


void ecodeAtomBlock::clear()
{
  int i;
  for (i=0; i<exec.size(); ++i)
    delete exec[i];
  exec.clear();
}

ecodeAtomBlock::~ecodeAtomBlock()
{
  clear();
}


evar ecodeAtomBlock::interpret(estrhashof<evar>& env)
{
  int i;
  for (i=0; i<exec.size()-1; ++i)
    exec[i]->interpret(env);
  if (exec.size())
    return(exec[i]->interpret(env));
  return(evar());
}
void ecodeAtomBlock::parse(const estr& str)
{
  if (!str.len()) return;

  exec.clear();
  ecodeAtom *atom;
  int i;

  i=0;
  while (i<str.len()){
    atom=getatom(str,i);
    if (atom)
      exec.add(atom);
  }
}

evar interpret_line(estrhashof<evar>& env,const estr& str);

class ecodeAtomArg : public ecodeAtom
{
 public:
  estr cond;
  virtual evar interpret(estrhashof<evar>& env);
};
evar ecodeAtomArg::interpret(estrhashof<evar>& env)
{
  ldinfo("ecodeAtomArg::interpret");
  return(interpret_line(env,cond));
}

class ecodeAtomSingle : public ecodeAtom
{
 public:
  eatom *root;
//  estr exec;
  bool showReturn;
  bool parse(const estr& code);
  virtual evar interpret(estrhashof<evar>& env);

  ecodeAtomSingle(): root(0x00) {}
  ~ecodeAtomSingle() { if (root) delete root; }
};

bool ecodeAtomSingle::parse(const estr& code)
{
  estrarray sa;
  if (!split_atoms2(code,sa)) return(false);

  root=new eatom(sa);
  estr s;
  root->print(s);
  ldinfo("interpret_line: command tree: "+s);
  return(true);
}

evar ecodeAtomSingle::interpret(estrhashof<evar>& env)
{
  ldinfo("ecodeAtomSingle::interpret");

/*
  if (!exec.len()) return(evar());

  estrarray sa;
  if (split_atoms2(exec,sa)){
    eatom root(sa);
    estr s;
    root.print(s);
    ldinfo("interpret_line: command tree: "+s);
    evar res(root.make(env));
    if (!res.isNull() && showReturn)
      cout << res << endl;
    return(res);
  }
  return(evar());
*/

  if (root==0x00) return(evar());

  root->clear();
  evar res(root->make(env));
  if (!res.isNull() && showReturn)
    cout << res << endl;
  return(res);
}

class ecodeAtomIf : public ecodeAtom
{
 public:
//  estr cond;
//  estr exectrue;
//  estr execfalse;

  ecodeAtom *cond;
  ecodeAtom *exectrue;
  ecodeAtom *execfalse;
  ecodeAtomIf(): cond(0x00),exectrue(0x00),execfalse(0x00) {}
  ~ecodeAtomIf() { if (cond) delete cond; if (exectrue) delete exectrue; if (execfalse) delete execfalse; }
  virtual evar interpret(estrhashof<evar>& env);
};
evar ecodeAtomIf::interpret(estrhashof<evar>& env)
{
  evar condval;
  condval.set(cond->interpret(env).convert(typeid(bool)));
//  condval = interpret_line(((ecodeAtomArg*)cond)->cond).convert<bool>().var;

  if (condval.isNull()){
    lwarn("condition is not boolean");
    return(evar());
  }

  if (condval.get<bool>())
    exectrue->interpret(env);
  else if (execfalse)
    execfalse->interpret(env);

  return(evar());
}



class ecodeAtomFor : public ecodeAtom
{
 public:
  ecodeAtom *init;
  ecodeAtom *loop;
  ecodeAtom *cond;

  ecodeAtom *exec;

  ecodeAtomFor(): init(0x00),loop(0x00),cond(0x00),exec(0x00) {}
  ~ecodeAtomFor(){ if (init) delete init; if (loop) delete loop; if (cond) delete cond; if (exec) delete exec; }
  virtual evar interpret(estrhashof<evar>& env);
};
evar ecodeAtomFor::interpret(estrhashof<evar>& env)
{
  init->interpret(env);

  evar condval;

  condval.set(cond->interpret(env).convert(typeid(bool)));
  if (condval.isNull()) { lwarn("condition is not boolean"); return(evar()); }
  while(condval.get<bool>()){
    exec->interpret(env);
    loop->interpret(env);
    condval.set(cond->interpret(env).convert(typeid(bool)));
    if (condval.isNull()) { lwarn("condition is not boolean"); return(evar()); }
  }
  return(evar());
}


class ecodeAtomWhile : public ecodeAtom
{
 public:
  ecodeAtom *cond;
  ecodeAtom *exec;

  ecodeAtomWhile(): cond(0x00),exec(0x00) {}
  ~ecodeAtomWhile() { if (cond) delete cond; if (exec) delete exec; }
  virtual evar interpret(estrhashof<evar>& env);
};
evar ecodeAtomWhile::interpret(estrhashof<evar>& env)
{
  evar condval;

  condval.set(cond->interpret(env).convert(typeid(bool)));
  if (condval.isNull()) { lwarn("condition is not boolean"); return(evar()); }
  
//  condval = interpret_line(((ecodeAtomArg*)cond)->cond);
  while(condval.get<bool>()){
    exec->interpret(env);
    condval.set(cond->interpret(env).convert(typeid(bool)));
    if (condval.isNull()) { lwarn("condition is not boolean"); return(evar()); }
//    condval = interpret_line(((ecodeAtomArg*)cond)->cond);
  }
  return(evar());
}


class ecodeAtomDo : public ecodeAtom
{
 public:
  ecodeAtom *cond;
  ecodeAtom *exec;
  ecodeAtomDo(): cond(0x00),exec(0x00) {}
  ~ecodeAtomDo() { if (cond) delete cond; if (exec) delete exec; }
  virtual evar interpret(estrhashof<evar>& env);
};
evar ecodeAtomDo::interpret(estrhashof<evar>& env)
{
  evar condval;

  do{
    exec->interpret(env);
//    condval = interpret_line(((ecodeAtomArg*)cond)->cond);
    condval.set(cond->interpret(env).convert(typeid(bool)));
    if (condval.isNull()) { lwarn("condition is not boolean"); return(evar()); }
  }while(condval.get<bool>());
  return(evar());
}


//#include "efuncbase.h"


class efuncCode : public efuncBase
{
 public:
  estr args;
  ecodeAtom *exec;

  efuncCode(): exec(0x00) {}
  ~efuncCode() { if (exec) delete exec; }

  inline const type_info& getTypeid(){ return(typeid(estr)); }
  evar call(const evararray &args);
  void updateInfo(efunc *f);
};
evar efuncCode::call(const evararray &argsarr)
{
  estrhashof<evar> env;
  env += getParser()->objects;
  env.add("args",evar(argsarr));

  int i;
  estrarray argnames(args);
  for (i=0; i<argnames.size(); ++i){
    if (i>=argsarr.size()) break;
    env.add(argnames[i],argsarr[i]);
  }

  return(exec->interpret(env));
}
void efuncCode::updateInfo(efunc *f)
{
  // TODO: setup info on variables
}


class ecodeAtomFunction : public ecodeAtom
{
 public:
  estr name;
  ecodeAtom *args;
  ecodeAtom *exec;
//  ecodeAtomFunction(): args(0x00),exec(0x00) {}
//  ~ecodeAtomFunction(){ if (args) delete args; if (exec) delete exce; }
  virtual evar interpret(estrhashof<evar>& env);
};
evar ecodeAtomFunction::interpret(estrhashof<evar>& env)
{
  if (!parser->funcs.exists(name))
    parser->funcs.add(name,earray<efunc>());

  efuncCode *funcCode = new efuncCode;
  funcCode->exec=exec;
  if (args->type==CA_ARG){
    funcCode->args=static_cast<ecodeAtomArg*>(args)->cond;
  }else
    ldwarn("argument block of function definition is not (argument) type");

  efunc func;
//  func.func=funcCode;
//  ++funcCode->pcount;
  func.setFunc(funcCode);

  parser->funcs[name].add(func);
  // register function
  return(evar());
}


/*
void getstratom(const estr& str,int ind,ecodeAtom& catom)
{
  int i;

  for (i=ind+1; i<str.len() && str[i]!='"'; ++i){
    if (str[i]=='\\') ++i;
  }
  catom.text=str.substr(ind,1+i-ind);
  catom.type=CA_STR;
}

void getnumatom(const estr& str,int ind,ecodeAtom& catom)
{
  int i;
  bool dot;
  bool exp;
  bool hex;

  dot=false; exp=false; hex=false;

  i=ind;
  if (str[i]=='-') ++i;
  for (; i<str.len() && (str[i]>=0 && str[i]<=9 || !dot  && str[i]=='.' || !exp && (str[i]=='e'||str[i]=='E') || !hex && str[i]=='x' || hex && (str[i]>='A' && str[i]<='F' || str[i]>='a' && str[i]<='f')); ++i){
    if (str[i]=='.') dot=true;
    if (str[i]=='e' || str[i]=='E'){
      exp=true;
      if (i+1<str.len() && str[i+1]=='-') ++i;
    }
    if (str[i]=='x')
      hex=true;
  }
  catom.text=str.substr(ind,i-ind);
  catom.type=CA_NUM;
}
*/

void skipstr(const estr& str,int& ind)
{
  ++ind;
  for (; ind<str.len() && str[ind]!='"'; ++ind){
    if (str[ind]=='\\') ++ind;
  }
}


ecodeAtom *getargatom(const estr& str,int& ind)
{
  int i;
  int count;

  count=0;
  for (i=ind+1; i<str.len() && (str[i]!=')' || count); ++i){
    if (str[i]=='"')
      skipstr(str,i);
    if (str[i]=='(') ++count;
    if (str[i]==')') --count;
  }

  ecodeAtomArg *aatom = new ecodeAtomArg;
  aatom->cond=str.substr(ind+1,i-ind-1);
  aatom->type=CA_ARG;
  ind=i+1;
  ldinfo("code arg atom: "+aatom->cond);
  return(aatom);
}

ecodeAtom *getblockatom(const estr& str,int& ind)
{
  int i;
  int count;

  ecodeAtomBlock *batom = new ecodeAtomBlock;
  count=0;
  for (i=ind+1; i<str.len() && (str[i]!='}' || count); ++i){
    if (str[i]=='"')
      skipstr(str,i);
    if (str[i]=='{') ++count;
    if (str[i]=='}') --count;
  }
  batom->parse(str.substr(ind+1,i-ind-1));
  batom->type=CA_CODE;
  ldinfo("code block atom: "+str.substr(ind+1,i-ind-1));
  ind=i+1;
  return(batom);
}

void getcontrolstr(const estr& str,int& ind,estr& control)
{
  int i;
  while (str[ind]==' ' && ind<str.len()) ++ind;
  for (i=ind; i<str.len() && (str[i]>='a' && str[i]<='z' || str[i]>='A' && str[i]<='Z' || str[i]>='0' && str[i]<='9' || str[i]=='_'); ++i){
  }
  control=str.substr(ind,i-ind);
  ind=i;
}

ecodeAtom *getcodeatom(const estr& str,int& ind)
{
  estr control;

  int i;
  i=ind;

  getcontrolstr(str,ind,control); 
  if (control=="if")
    return(getcodeifatom(str,ind));
  else if (control=="for")
    return(getcodeforatom(str,ind));
  else if (control=="while")
    return(getcodewhileatom(str,ind));
  else if (control=="do")
    return(getcodedoatom(str,ind));
  else if (control=="function")
    return(getcodefunctionatom(str,ind));
  else{ // not a statement control keyword, use line evaluator
    ind=i;
    return(getsingleatom(str,ind));
  }
}

ecodeAtom *getsingleatom(const estr& str,int& ind)
{
  int i;

  for (i=ind; i<str.len() && str[i]!=';'; ++i){
    if (str[i]=='"')
      skipstr(str,i);
  }

  ecodeAtomSingle* satom = new ecodeAtomSingle();

  int trimleft=0;
  if (i>0 && str[i-1]==';') trimleft=1;
  while (i-1-trimleft>ind && str[i-1-trimleft]==' ') ++trimleft;

  estr tmpstr(str.substr(ind,i-ind-trimleft));
  satom->parse(tmpstr);
//  satom->exec=tmpstr;
  satom->type=CA_CODE;
  satom->showReturn=false;
  if (i>=str.len()) satom->showReturn=true;

  ind=i+1;
  ldinfo("single atom: "+tmpstr);
  return(satom);
}

ecodeAtom *getatom(const estr& str,int& ind)
{
/*  if (str[ind]='"')
    getstratom(str,ind,catom);
  else if (str[ind]>='0' && str[ind]<='9' || str[ind]=='-')
    getnumatom(str,ind,catom);*/

  while (str[ind]==' ' && ind<str.len()) ++ind;

  if (ind>=str.len()) return(0x00);

  if (str[ind]=='"' || str[ind]>='0' && str[ind]<='9' || str[ind]=='-')
    return(getsingleatom(str,ind));
  else if (str[ind]=='(')
    return(getargatom(str,ind));
  else if (str[ind]=='{')
    return(getblockatom(str,ind));
  else
    return(getcodeatom(str,ind));
}

ecodeAtom *getcodeifatom(const estr& str,int& i)
{
  ecodeAtomIf *ifatom = new ecodeAtomIf;

  ifatom->cond=getatom(str,i);
  lerrorifr(!ifatom->cond || ifatom->cond->type!=CA_ARG,"\"if\" missing condition",(0x00));

  ifatom->exectrue=getatom(str,i);
  lerrorifr(!ifatom->exectrue || ifatom->exectrue->type==CA_ARG,"condition found where statement was expected",(0x00));

  ifatom->execfalse=0x00;

  estr control;
  int itmp;
  itmp=i;
  getcontrolstr(str,i,control);
//  cout << " > if : getcontrolstr = "<<control<<endl;
  if (control=="else"){
    ifatom->execfalse = getatom(str,i);
    lerrorifr(!ifatom->execfalse || ifatom->execfalse->type==CA_ARG,"condition found where statement was expected",(0x00));
  }else
   i=itmp;

  return(ifatom);
}

ecodeAtom *getcodeforatom(const estr& str,int& i)
{
  ecodeAtomFor *foratom = new ecodeAtomFor;

  ecodeAtom *tmpatom;

  tmpatom=getatom(str,i);
  lerrorifr(!tmpatom || tmpatom->type!=CA_ARG,"\"for\" missing condition",(0x00));

  int tmpi;

  tmpi=0;
  foratom->init=getatom(((ecodeAtomArg*)tmpatom)->cond,tmpi);
  lerrorifr(!foratom->init || foratom->init->type==CA_ARG,"\"for\" missing init part",(0x00));

  foratom->cond=getatom(((ecodeAtomArg*)tmpatom)->cond,tmpi);
  lerrorifr(!foratom->cond || foratom->cond->type==CA_ARG,"\"for\" missing condition part",(0x00));
   
  foratom->loop=getatom(((ecodeAtomArg*)tmpatom)->cond+";",tmpi);
  lerrorifr(!foratom->loop || foratom->loop->type==CA_ARG,"\"for\" missing loop part",(0x00));

  delete tmpatom;
 
  foratom->exec=getatom(str,i);
  lerrorifr(!foratom->exec || foratom->exec->type==CA_ARG,"condition found where statement was expected",(0x00));

  return(foratom);
}

ecodeAtom *getcodewhileatom(const estr& str,int& i)
{
  ecodeAtomWhile *whileatom = new ecodeAtomWhile;

  whileatom->cond=getatom(str,i);
  lerrorifr(!whileatom->cond || whileatom->cond->type!=CA_ARG,"\"while\" missing condition",(0x00));

  whileatom->exec=getatom(str,i);
  lerrorifr(!whileatom->exec || whileatom->exec->type==CA_ARG,"condition found where statement was expected",(0x00));

  return(whileatom);
}

ecodeAtom *getcodedoatom(const estr& str,int& i)
{
  ecodeAtomDo *doatom = new ecodeAtomDo;

  doatom->exec=getatom(str,i);
  lerrorifr(!doatom->exec || doatom->exec->type==CA_ARG,"\"do\" condition found where statement was expected",(0x00));

  estr control;
  getcontrolstr(str,i,control);
  lerrorifr(control!="while","\"do\" missing \"while\" keyword",(0x00));

  doatom->cond=getatom(str,i);
  lerrorifr(!doatom->cond || doatom->cond->type!=CA_ARG,"\"do\" statement found where condition was expected",(0x00));
 
  return(doatom);
}

ecodeAtom *getcodefunctionatom(const estr& str,int& i)
{
  ecodeAtomFunction *funcatom = new ecodeAtomFunction;

  getcontrolstr(str,i,funcatom->name);
  
  funcatom->args=getatom(str,i);
  lerrorifr(!funcatom->args || funcatom->args->type!=CA_ARG,"\"function\" statement found where args were expected",(0x00));
 
  funcatom->exec=getatom(str,i);
  lerrorifr(!funcatom->exec || funcatom->exec->type==CA_ARG,"\"function\" args found where statement was expected",(0x00));

  return(funcatom);
}

evar code_interpret(estrhashof<evar>& env,const estr& str)
{
  if (!str.len()) return(evar());

  ecodeAtomBlock code;
  ldinfo("code_interpret");
  code.parse(str);
  return(code.interpret(env));
}

evar interpret_line(estrhashof<evar>& env,const estr& str)
{
  if (!str.len()) return(evar());
  estrarray sa;
  if (split_atoms2(str,sa)){
    eatom root(sa);
    estr s;
    root.print(s);
    ldinfo("interpret_line: command tree: "+s);
    evar res;
    res.copy(root.make(env));
    return(res);
  }
  return(evar());
}

#ifdef EUTILS_HAVE_READLINE_H
void my_rlhandler(char* line);
#endif


estr exechost;

evar epinterpret(const estr& str)
{
  estr tmpstr(str);
  estr tmpexechost;
  int i;

  tmpstr.trim();
  if (!tmpstr.len()) return(evar());

  return(code_interpret(getParser()->objects,tmpstr));
}

#include "efile.h"
#include "esystem.h"

#ifdef EUTILS_HAVE_READLINE_H
 #include "edir.h"
 

estr histfile;


void my_rlhandler(char* line)
{
  if (line==NULL){
    cout << endl;
    rl_callback_handler_remove();
    lerrorif(write_history(histfile._str)!=0,"unable to write history to file: "+histfile);
    exit(0);
    // Ctrl-D will allow us to exit nicely
  }else{
    if(*line!=0){
      // If line wasn't empty, store it so that uparrow retrieves it
      epinterpret(line);
      add_history(line);
    }
//    printf("Your input was:\n%s\n", line);
    free(line);
  }
}

char *dupstr(char *s)
{
  char *r;
  r = new char[strlen(s)+1];
  strcpy(r,s);
  return(r);
}

char *eparser_command_generator(const char *text,int state)
{
  static int list_index, len;
  char *name;

  /* If this is a new word to complete, initialize now.  This includes
     saving the length of TEXT for efficiency, and initializing the index
     variable to 0. */
  if (!state) {
    list_index=0;
    len=strlen(text);
  }

  /* Return the next name which partially matches from the command list. */
  for (; list_index<getParser()->funcs.size(); ++list_index){
    name = getParser()->funcs.keys(list_index)._str;
    if (strncmp(name,text,len) == 0){
      ++list_index;
      return(dupstr(name));
    }
  }
  for (; list_index-getParser()->funcs.size()<getParser()->objects.size(); ++list_index){
    name = getParser()->objects.keys(list_index-getParser()->funcs.size())._str;
    if (strncmp(name,text,len) == 0){
      ++list_index;
      return(dupstr(name));
    }
  }
  return((char*)NULL);
}

char **eparser_completion(const char *text,int start,int end)
{
  char **matches;
  matches = (char **)NULL;
  if (start == 0)
    matches = rl_completion_matches(text, eparser_command_generator);
  return (matches);
}
#endif

void interpretGotInput()
{
#ifdef EUTILS_HAVE_READLINE_H
  rl_callback_read_char();
#else
  estr cmd;
  efile input(stdin);
  if (exechost.len())
    cout << "easyc++@"<< exechost << "> ";
  else
    cout << "easyc++> ";
  flush(cout);
  input.readln(cmd);
  if (cmd[0]==0x00) exit(0);
  if (cmd[cmd.size()-1]==0x0A)
    cmd.del(-1);
  epinterpret(cmd);
#endif
}

void epruninterpret(int argvc,char **argv)
{
  epregisterFunctions();

#ifdef EUTILS_HAVE_READLINE_H
  using_history();

  histfile=env()["HOME"]+"/."+efile(argv[0]).basename()+"_history";
  lerrorif(read_history(histfile._str)!=0,"unable to read history from "+histfile);

  rl_callback_handler_install("easyc++> ", (rl_vcpfunc_t*) &my_rlhandler);
  rl_attempted_completion_function = (char**(*)(const char*,int,int))eparser_completion;  
#endif

  getSystem()->addReadCallback(0,interpretGotInput,evararray());
  getSystem()->run();

#ifdef EUTILS_HAVE_READLINE_H
  rl_callback_handler_remove();
#endif
  cout << endl;
}

#include "eregexp.h"

void epinterpretfile(const estr& file)
{
  efile f(file);
  
  eregexp re_comment("//");
  int b,e;
  estr line;
  while (f.readln(line)){
    if (line.len()==0 || line[0]=='#') continue;
    if (re_comment.match(line,0,b,e)>=0)
      line.del(b);
    epinterpret(line);
  }
}

