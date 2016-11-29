///General structures for Bayesian analysis
///
///This is a set of interfaces for objects needed in Bayesian analysis.
///The As of June2015, the interfaces are in early stages of development.
///John G Baker - NASA-GSFC (2015)

#ifndef PTMCMC_BAYESIAN_HH
#define PTMCMC_BAYESIAN_HH
//#include <valarray>
//#include <vector>
//#include <sstream>
//#include <cmath>
//#include <iostream>
//#include <memory>
#include "options.hh"
#include "states.hh"
#include "probability_function.hh"

using namespace std;

///Provides some functions/interface common to all bayesian components (data, signal, likelihood).
///
///In particular these all implement the stateSpaceInterface, and the Optioned interfaces.
///Derived classes generally provide:
///-addOptions(Options,String):[options.hh]Object defines its (de facto) command-line options. Begin with call to GLens::addOptions(Options,String).
///-setup():Object sets itself up after options have been parsed. Should must set nativeSpace and can define prior with setPrior(). End with call to haveSetup().
///-defWorkingStateSpace():[states,hh,required] Object finds location of its parameters in the state space. End by calling haveWorkingStateSpace().
///-setState(State): Object takes values from its parameters as needed for state-specific computations. Begin with haveWorkingStateSpace().
class bayes_component: public stateSpaceInterface,public Optioned{
  bool have_setup;
  bool have_prior;
protected:
  stateSpace nativeSpace;
  shared_ptr<const sampleable_probability_function> nativePrior;
  bayes_component(string typestring="null",string option_name="",string option_info=""):typestring(typestring),option_name(option_name),option_info(option_info){have_setup=false;have_prior=false;};
  ~bayes_component(){};
  ///This declares that setup is complete.
  void haveSetup(){have_setup=true;};
  void setPrior(sampleable_probability_function* prior){
    nativePrior.reset(prior);have_prior=true;
    cout<<"bayes_component::setPrior: Setting for this object("<<typeid(*this).name()<<")"<<endl;
  };
  ///This assert checks that the object is already set up.
  bool checkSetup(bool quiet=false)const{
    if((!quiet)&&!have_setup){
      cout<<"bayes_component::checkSetup: Cannot apply object before setup. Be sure to call haveSetup() when set-up is complete."<<endl;
      exit(1);
    }
    return have_setup;
  };
  ///The following are for use with bayes_component_selector
  string typestring;
  string option_name;
  string option_info;
public:
  ///Return a pointer to an appropriate prior for this objects stateSpace
  virtual shared_ptr<const sampleable_probability_function> getObjectPrior()const{
    if(have_prior)return nativePrior;
    else { cout<<"bayes_component::getObjectPrior: No prior is defined for this object("<<typeid(*this).name()<<")!"<<endl;exit(1);}
  };
  virtual const stateSpace* getObjectStateSpace()const{checkSetup();return &nativeSpace; };
  virtual void setup(){};
  virtual string show_pointers(){
    ostringstream ss;
    if(have_prior){
      ss<<"nativePrior["<<nativePrior.get()<<","<<nativePrior.use_count()<<"]";
    } else ss<<"nativePrior[null]";
    return ss.str();
  };
  ///The following are to support bayes_component_selector
  string get_typestring(){return typestring;};
  string get_option_name(){return option_name;};
  string get_option_info(){return option_info;};
  bool type_matches(const bayes_component *other)const{
    //cout<<"bayes_component_selector::type_matches: Comparing type '"<<typestring<<"' with '"<<other->typestring<<"' -> "<<(other->typestring==typestring)<<endl;
    if(typestring=="null")cout<<"bayes_component: Cannot match 'null' type."<<endl;return other->typestring==typestring;
  };  
};


///Class to provide runtime selection among several compatible component objects
class bayes_component_selector : public Optioned {
  vector<bayes_component*>components;
  bool required;
public:
  bayes_component_selector( const vector<bayes_component*> &list,bool require=false):required(require){//Note: the argument must be a persistent vector of pointers to the required selectable component objects
    if(list.size()<1){
      cout<<"bayes_component selector::(constructor): Cannot select from an empty list!"<<endl;
      exit(1);
    } 
    string compttype=list[0]->get_typestring();
    if(compttype=="null"){
      cout<<"bayes_component selector::(constructor): Cannot select from 'null' type components!"<<endl;
      exit(1);
    }
    for (auto const &component : list){
      if(not component->type_matches(list[0])){
	cout<<"bayes_component selector::(constructor): Type of component "<<components.size()<<" does not match type of first component. Skipping!"<<endl;
      } else {
	components.push_back(component);
      }
    }
  };
  void addOptions(Options &opt,const string &prefix=""){
    Optioned::addOptions(opt,prefix);
    for(auto const &comp : components){
      if(comp->get_option_name()==""){
	cout<<"bayes_component selector::addOptions: A component of type "<<components.size()<<" has an empty option name!"<<endl;
	exit(1);
      }
      addOption(comp->get_option_name(),comp->get_option_info());
    }
  };
  bayes_component * select(Options &opt){
    for(auto const &comp : components){
      if(opt.set(comp->get_option_name())){
	comp->addOptions(opt);
	return comp;
      }
    }
    //If here is reached no option was found.
    if(required){
      string flags="{ ";
      for(auto const &comp : components)flags+=comp->get_option_name()+" ";
      flags+="}";
      cout<<"Cannot select '"<<components[0]->get_typestring()<<"' component.  No flag was provided from required set: "<<flags<<"."<<endl;
      cout<<"Available flags are:\n"<<opt.print_usage()<<endl;
    } else { //default to first option
      if(components.size()<1){
	cout<<"bayes_component_selector::select: Cannot select from empty list!"<<endl;
	exit(1);
      }
      auto comp=components[0];
      comp->addOptions(opt);
      return comp;
    }
  }
};
	

///Interface class for bayesian signal data. This is some kind of compound data.
///We begin with only what we need for ptmcmc, that we can write the signal
class bayes_signal : public bayes_component {

public:
  virtual vector<double> get_model_signal(const state &st, const vector<double> &labels)const=0;
  ///Stochastic signals imply some variance, default is to return 0
  //virtual double getVariance(double tlabel){return 0;};
  virtual vector<double> getVariances(const state &st,const vector<double> tlabel)const{
    //cout<<"bayes_signal::getVariances"<<endl;
    return vector<double>(tlabel.size(),0);};
};

///Interface class for bayesian signal data. This is some kind of compound data.
///We begin with only what we need for ptmcmc, that we can write the signal
//class bayes_old_signal {
//public:
//  virtual int size()=0;
//  virtual void write(ostream &out,state &st, int nsamples=-1, double tstart=0, double tend=0)=0;
//};

///Interface class for bayesian signal data. This is some kind of compound data.
///
///The label space is treated as one-dimensional, though this may be 2-D as
///for image data, it could also be time or freq.
///dvalue is a scale for errors.
class bayes_data : public bayes_component {
  int have_data;
protected:
  vector<double>labels,values,dvalues;
  double label0;
  //bool have_model;
  bool allow_fill;
public:
  enum {LABELS=1,VALUES=2,DVALUES=4};
  //bayes_data():label0(0),have_model(false),have_data(0),allow_fill(false){};
  bayes_data():label0(0),have_data(0),allow_fill(false){};
  vector<double>getLabels()const{return labels;};
  void getDomainLimits(double &start, double &end)const{
    if(labels.size()==0){
      cout<<"bayes_data::getDomainLimits: Cannot get limit on empty object."<<endl;
      exit(1);
    }
    start=labels.front();
    end=labels.back();
  };
  virtual int size()const{return labels.size();};
  virtual bool can_mock(){return allow_fill;};
  virtual double getFocusLabel(bool original=false)const{return 0;};
  virtual double getValue(int i )const{return values[i];};
  virtual vector<double>getValues()const{checkData(VALUES);return values;};
  virtual vector<double>getDeltaValues()const{checkData(DVALUES);return dvalues;};
  //virtual double getVariance(int i)const=0;
  virtual vector<double> getVariances(const state &s)const{
    checkData(DVALUES);
    //cout<<"bayes_data::getVariances"<<endl;
    //    for(int i=0;i<labels.size();i++)
    //      cout<<i<<","<<labels[i]<<": "<<values[i]<<","<<dvalues[i]<<endl;
    vector<double>v=getDeltaValues();
    for(int i=0;i<v.size();i++){
      v[i]*=v[i];
      //cout<<i<<":"<<v[i]<<endl;
    }
    return v;
  };
  ///Declare the aspects of the data that are available.
  ///Depending on the nature of the data, it may include labels, values, and/or dvalues
  ///Different operations may need different such elements.
  void haveData(int type=LABELS|VALUES|DVALUES){have_data = have_data | type;};
  //Some data come with their own modeled parameters.
  //virtual void set_model(state &st){have_model=true;};
  ///Check that required types of data are ready and fail otherwise.
  virtual void assertData(int test)const{
    if(0==(have_data & test)){
      cout<<"bayes_data::checkData: Cannot operate on data before it is loaded."<<endl;
      int missing = test & ~ have_data;
      if(missing & LABELS )cout<<"  *labels* are missing."<<endl;
      if(missing & VALUES )cout<<"  *values* are missing."<<endl;
      if(missing & DVALUES )cout<<"  *dvalues* are missing."<<endl;
      exit(1);
    }
  };
  ///Check that required types of data are ready and return result.
  virtual bool checkData(int test)const{
    if(0==(have_data & test))return false;
    return true;
  };
  void fill_data(vector<double> &newvalues){
    assertData(LABELS|VALUES|DVALUES);
    if(!allow_fill){
      cout<<"bayes_data::fill_data: Operation not permitted for this class object/instance."<<endl;
      exit(-1);
    }
    //test the vectors
    if(newvalues.size()!=labels.size()){
      cout<<"bayes_data::fill_data: Input array is of the wrong size."<<endl;
      exit(-1);
    }
    for(int i=0;i<labels.size();i++){
      values[i]=newvalues[i];
    }
  };  
  ///By default assume there are no parameters associated with the data so this is trivial.  It should be overloaded if there are data parameters.
  virtual void defWorkingStateSpace(const stateSpace &sp){
    checkSetup();//Call this assert whenever we need options to have been processed.
    haveWorkingStateSpace();
  };
};

///Bayes class for a likelihood function object
///
///The interface here is probably not everything we want, but is enough for what was already in the main function.
class bayes_likelihood : public probability_function, public bayes_component {
  double like0;
protected:
  bayes_signal *signal;
  bayes_data *data;
  double best_post;
  state best;  
public:
  bayes_likelihood(stateSpace *sp,bayes_data *data,bayes_signal *signal):probability_function(sp),data(data),signal(signal),like0(0){
    if(sp)best=state(sp,sp->size());
    reset();
};
  //The latter is just a hack for testing should be removed.
  //bayes_likelihood(stateSpace *sp):probability_function(sp),like0(0){};
  ///Hard check that the signal and data are non-null
  virtual void checkPointers()const{
    if(!data||!signal){
      cout<<"bayes_likelihood::checkPointers(): Cannot operate on undefined pointers!"<<endl;
      exit(1);
    }
  };
  //Overloaded from bayes_component
  virtual void setup(){
    haveSetup();
    ///Set up the output stateSpace for this object
    checkPointers();
    nativeSpace=*data->getObjectStateSpace();
    nativeSpace.attach(*signal->getObjectStateSpace());
    //Set the prior...
    setPrior(new independent_dist_product(&nativeSpace,data->getObjectPrior().get(),signal->getObjectPrior().get()));
    space=&nativeSpace;
    best=state(space,space->size());
    //Unless otherwise externally specified, assume nativeSpace as the parameter space
    defWorkingStateSpace(nativeSpace);
  };
  int size()const{return data->size();};
  virtual void reset(){
    best_post=-INFINITY;
    best=best.scalar_mult(0);
  }
  virtual state bestState(){return best;};
  virtual double bestPost(){return best_post;};
  //virtual bool setStateSpace(stateSpace &sp)=0;
  virtual double getFisher(const state &s0, vector<vector<double> >&fisher_matrix){
    cout<<"getFisher not implemented for bayes_likelihood object ("<<typeid(*this).name()<<")"<<endl;
  };
  virtual vector<double> getVariances(const state &st)const{
    checkPointers();
    const vector<double>labels=data->getLabels();
    vector<double>var=data->getVariances(transformDataState(st));
    vector<double>svar=signal->getVariances(transformSignalState(st),labels);
    //cout<<"bayes_like::getVariances: var.size="<<var.size()<<" svar.size="<<svar.size()<<endl;
    for(int i=0;i<data->size();i++)var[i]+=svar[i];
    return var;
  };
  ///from stateSpaceInterface
  virtual void defWorkingStateSpace(const stateSpace &sp){
    checkSetup();//Call this assert whenever we need options to have been processed.
    haveWorkingStateSpace();
    checkPointers();
    data->defWorkingStateSpace(sp);
    signal->defWorkingStateSpace(sp);
  };
  //virtual void set_model(state &st){};
  //virtual void write(ostream &out,state &st, int nsamples=-1, double tstart=0, double tend=0)=0;
  ///This is a function for writing out the data and model values and error bars.
  ///The dummy version here may work for many applications, or the function may be overloaded to suit a specific application
  virtual void write(ostream &out,state &st){
    checkWorkingStateSpace();//Call this assert whenever we need the parameter index mapping.
    checkPointers();
    data->assertData(data->LABELS);
    bool have_vals=data->checkData(data->VALUES);
    bool have_dvals=data->checkData(data->DVALUES);
    double xfoc=0,x0=0;  
    std::vector<double> dmags,dvar;
    std::vector<double>xs=data->getLabels();
    if(have_vals){
      xfoc=data->getFocusLabel();
      x0=data->getFocusLabel(true);
    }
    std::vector<double> model=signal->get_model_signal(transformSignalState(st),xs);
    if(have_dvals){
      dmags=data->getDeltaValues();
      dvar=getVariances(st);
    }
    
    for(int i=0;i<xs.size();i++){
      double S=0;
      if(have_dvals)S=dvar[i];
      if(i==0){
	out<<"#x"<<" "<<"x_vs_xfoc" 
	   <<" "<<"model_val";
	if(have_vals)  out<<" "<<"data_val";
	if(have_dvals) out<<" "<<"model_err"<<" "<<"data_err";
	out<<endl;
      }
      out<<xs[i]+x0<<" "<<xs[i]-xfoc
	 <<" "<<model[i];
      if(have_vals)  out<<" "<<data->getValue(i);
      if(have_dvals) out<<" "<<sqrt(S)<<" "<<dmags[i];
      out<<endl;
    }
   
  };
  ///This is a function for writing out the model values and error bars on a grid, generally imagined to be finer-grained than
  ///the data values. In particular, this allows examining the model where it fills in gaps in the data.
  ///The dummy version here may work for many applications, or the function may be overloaded to suit a specific application.
  virtual void writeFine(ostream &out,state &st,int samples=-1, double xstart=0, double xend=0){
    std::vector<double>xs=data->getLabels();
    if(samples<0)getFineGrid(samples,xstart,xend);
    double delta_x=(xend-xstart)/(samples-1.0);
    for(int i=0;i<samples;i++){
      double x=xstart+i*delta_x;
      xs.push_back(x);
    }
    double xfoc=data->getFocusLabel();
    double x0=data->getFocusLabel(true);
    
    vector<double> model=signal->get_model_signal(transformSignalState(st),xs);
    vector<double> dmags=data->getDeltaValues();
    vector<double> dvar=getVariances(st);

    for(int i=0;i<xs.size();i++){
      double x=xs[i];
      if(i==0)
	out<<"#x"<<" "<<"x_vs_xfoc" 
	   <<" "<<"model_mag"
	   <<endl;
      out<<x+x0<<" "<<x-xfoc
	 <<" "<<model[i]
	 <<endl;
    }
  };
  ///This function defines the grid used by writeFine (by default). 
  ///The dummy version here may work for many applications, or the function may be overloaded to suit a specific application.
  void getFineGrid(int & nfine, double &xfinestart, double &xfineend)const{
    checkPointers();
    nfine=data->size()*2;
    double x0,xstart,xend;
    data->getDomainLimits(xstart,xend);
    x0=data->getFocusLabel();
    double finewidthfac=1.5;
    xfinestart=x0-(-xstart+xend)*finewidthfac/2.0;
    xfineend=x0+(-xstart+xend)*finewidthfac/2.0;
  };
protected:
  //Some standard Likelihood models
  ///Chi-squared likelihood models independent gaussian random noise for each datum.
  ///For a normalized gaussian distribution:
  /// logL = Exp[-(x-\mu)^2/Var] - (1/2)ln(2\pi Var) 
  ///summed over all data values x(label).  Typically the model mean value \mu depends on label and on the parameters and variance Var
  ///may also depend on parameters and label.
  double log_chi_squared(state &s)const{
    checkPointers();
    checkSetup();
    data->assertData(data->LABELS|data->VALUES|data->DVALUES);
    //here we assume the model data is magnitude as function of time
    //and that dmags provides a 1-sigma error size.
    double sum=0;
    double nsum=0;
    vector<double>tlabels=data->getLabels();
    vector<double>modelData=signal->get_model_signal(transformSignalState(s),tlabels);
    vector<double>S=getVariances(s);
#pragma omp critical
    {
      for(int i=0;i<tlabels.size();i++){
        double d=modelData[i]-data->getValue(i);
        //cout<<"i,d,S:"<<i<<","<<d<<","<<S[i]<<endl;
        sum+=d*d/S[i];
        nsum+=log(S[i]);  //note trying to move log outside loop can lead to overflow error.
      }
    }
    //cout<<" sum="<<sum<<"  nsum="<<nsum<<endl;
    sum+=nsum;
    sum/=-2;
    //cout<<" sum="<<sum<<"  like0="<<like0<<endl;
    //cout<<"this="<<this<<endl;
    return sum-like0;
  };
  double log_poisson(state &s)const{
    checkPointers();
    checkSetup();
    data->assertData(bayes_data::LABELS);
    //here we assume the model data are rates at the data label value points
    //and the extra last entry in model array is the total expected rate
    double sum=0;
    vector<double>labels=data->getLabels();
    vector<double>modelData=signal->get_model_signal(transformSignalState(s),labels);
    int ndata=labels.size();
#pragma omp critical
    {
      for(int i=0;i<ndata;i++){
	sum += log(modelData[i]);
      }
      sum -= modelData[ndata];//For Poisson data the model array should end with the total number of events expected.
    }
    return sum-like0;
  };
  ///The Fisher information matrix for the chi-squared likelihood.
  ///Generally the fisher information should return the expected value E[\partial_i(logL) \partial_j(logL)]
  ///As noted, for a normalized gaussian distribution:
  /// logL = -(x-\mu)^2/Var - (1/2)ln(2\pi Var) 
  ///summed over all data values x(label).  Typically the model mean value \mu depends on label and on the parameters and variance Var
  ///may also depend on parameters and label. For the Fisher computation we need:
  ///  \partial_i logL =  (x-\mu)/Var \partial_i \mu + (1/2)( (x-\mu)^2/Var - 1 ) \partial_i ln(Var)
  /// To compute the Fisher matrix, note that E[(x-mu)^n] vanishes for odd n, equals Var for n=2 and equals 3Var^2 for n=4.
  /// Thus:
  ///   F_{ij} =  (1/2) \partial_i ln(Var) \partial_j ln(Var) + (1/Var) \partial_i \mu \partial_j \mu  
  /// Computationally, the trick is to get values for the derivatives. This requires some estimate for the relevant scale.
  /// We apply a boot-strap approach, first estimating based on some scale-factor times smaller than the prior scales.
  /// Note that his computation is not highly efficient. It is expected to be called relatively few times, certainly not
  /// at each step of an MCMC chain.  It requires several times more than dim^2 model evaluations.
  double getFisher_chi_squared(const state &s0, vector<vector<double> >&fisher_matrix){
    int dim=s0.size();
    int maxFisherIter=15*dim;
    double deltafactor=0.001;
    double tol=10*deltafactor*deltafactor*deltafactor;
    valarray<double> scales;
    nativePrior->getScales(scales);
    vector<double>labels=data->getLabels();
    double N=labels.size();
    vector<double>Var0=getVariances(s0);
    double err=1;
    int count=0;
    vector< vector<double> >last_fisher_matrix(dim,vector<double>(dim,0));
    while(err>tol&&count<maxFisherIter){
      for(int i=0;i<dim;i++){
	//compute i derivative of model
	vector<double>dmudi(N);
	vector<double>dVdi(N);
	double h=scales[i]*deltafactor;
	state sPlus=s0;
	sPlus.set_param(i,s0.get_param(i)+h);
	state sMinus=s0;
	sMinus.set_param(i,s0.get_param(i)-h);
	vector<double>modelPlus=signal->get_model_signal(transformSignalState(sPlus),labels);
	vector<double>modelMinus=signal->get_model_signal(transformSignalState(sMinus),labels);
	for(int k=0;k<N;k++)dmudi[k]=(modelPlus[k]-modelMinus[k])/h/2.0;
	//compute i derivative of Variances
	vector<double>VarPlus=getVariances(sPlus);
	vector<double>VarMinus=getVariances(sMinus);
	for(int k=0;k<N;k++)dVdi[k]=(VarPlus[k]-VarMinus[k])/h/2.0;
	

	for(int j=i;j<dim;j++){
	  //compute j derivative of model
	  vector<double>dmudj(N);
	  vector<double>dVdj(N);
	  h=scales[j]*deltafactor;
	  state sPlus=s0;
	  sPlus.set_param(j,s0.get_param(j)+h);
	  state sMinus=s0;
	  sMinus.set_param(j,s0.get_param(j)-h);
	  vector<double>modelPlus=signal->get_model_signal(transformSignalState(sPlus),labels);
	  vector<double>modelMinus=signal->get_model_signal(transformSignalState(sMinus),labels);
	  for(int k=0;k<N;k++)dmudj[k]=(modelPlus[k]-modelMinus[k])/h/2.0;
	  //compute i derivative of Variances
	  vector<double>VarPlus=getVariances(sPlus);
	  vector<double>VarMinus=getVariances(sMinus);
	  for(int k=0;k<N;k++)dVdj[k]=(VarPlus[k]-VarMinus[k])/h/2.0;

	  //Compute fisher matrix element
	  fisher_matrix[i][j]=0;
	  for(int k=0;k<N;k++) fisher_matrix[i][j] += ( dmudi[k]*dmudj[k] - dVdi[k]*dVdj[k]/2.0 ) / Var0[k];
	  fisher_matrix[j][i] = fisher_matrix[i][j];
	}
      }
      
      //estimate error
      err=0;
      double square=0;
      for(int i=0;i<dim;i++)for(int j=0;j<dim;j++){
	  double delta=(fisher_matrix[i][j]-last_fisher_matrix[i][j]);///scales[i]/scales[j];
	  square+=fisher_matrix[i][j]*fisher_matrix[i][j];
	  err+=delta*delta;
	}
      err/=square;
      //set scale estimate based on result
      for(int i=0;i<dim;i++)scales[i]=1.0/sqrt(1/scales[i]+fisher_matrix[i][i]);
      //prep for next version of fisher calc;
      for(int i=0;i<dim;i++)for(int j=0;j<dim;j++)last_fisher_matrix[i][j]=fisher_matrix[i][j];
      count++;
    }
    err=sqrt(err);
    cout<<"err="<<err<<endl;
    cout<<"tol="<<tol<<endl;
    if(err<tol)return tol;
    return err; 
  };
  
protected:
    
  ///A largely cosmetic adjustment to yield conventional likelihood level with noise_mag=0;
  virtual void set_like0_chi_squared(){
    checkPointers();
    data->assertData(data->DVALUES);
    like0=0;
    const vector<double>dv=data->getDeltaValues();
    for(int i=0; i<size(); i++){
      like0+=log(dv[i]*dv[i]);
    }
    like0/=-2;
    cout<<"bayesian_likelihood:Setting like0="<<like0<<endl;
    //cout<<"this="<<this<<endl;
  };
  ///May apply transformations to the Data/Signal states
  virtual state transformDataState(const state &s)const {return s;};
  ///May apply transformations to the Data/Signal states
  virtual state transformSignalState(const state &s)const {return s;};
  ///This method generates mock data and assigns it to the associated bayes_data object.
public:
  void mock_data(state &s){
    checkPointers();
    checkSetup();
    GaussianDist normal(0.0,1.0);
    //here we assume the model data is magnitude as function of time
    //and that dmags provides a 1-sigma error size.
    vector<double>labels=data->getLabels();
    //First we set up instrumental-noise free data. This is so that the data object can estimate noise
    vector<double>modelData=signal->get_model_signal(transformSignalState(s),labels);
    vector<double>Sm=signal->getVariances(s,labels);
    vector<double>values(labels.size());
    vector<double>dvalues(labels.size());
    for(int i=0;i<labels.size();i++){
      values[i]=modelData[i]+sqrt(Sm[i])*normal.draw();
    }
    data->fill_data(values);
  };
};

///Base class for defining a Bayesian sampler object
///
///To begin with the only option is for MCMC sampling, though we expect soon to add a multinest option.
class bayes_sampler : public Optioned {
public:
  bayes_sampler(){};
  virtual bayes_sampler * clone()=0;
  virtual int initialize()=0;
  virtual int run(const string & base, int ic=0)=0;
  ///This is too specific for a generic interface, but we're building on what we had before...
  //virtual int analyze(const string & base, int ic, int Nsigma, int Nbest, bayes_old_signal &data, double tfinestart, double tfineend)=0;
  virtual int analyze(const string & base, int ic, int Nsigma, int Nbest, bayes_likelihood &like)=0;
};


#endif

