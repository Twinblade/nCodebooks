
#include "experiment.h"


/*Optimze technique
 * use -pg compile flag
 * run as :
 * gprof -bp <program name > <output name>
 * 
 * 
 */

using namespace csvm;
using namespace std;

/*void showUsage(){
   cout << "CSVM: An experimental platform for the Convolutional Support Vector Machine architecture\n" <<
           "Usage: CSVM [settingsFile]\n" <<
           "Where:\n" <<
           "\tsettingsFile: location of settingsFile\n";
       
   
}*/

double run(char* settingsDir){
   
   /*if(argc!=2){
      showUsage();
      return 0;
   }*/
   
   CSVMClassifier c;
   ImageScanner scanner;
   vector<Patch> newPatches;
   vector<Patch> patches;
   LBPDescriptor localBinPat;

   //load settingsFile
   //c.setSettings("../build/settings");
   c.setSettings(settingsDir);
   
   
   
   //setup cifar10 data directories
   vector<string> imDirs;
   
   imDirs.push_back("../../datasets/cifar-10-batches-bin/data_batch_1.bin");
   imDirs.push_back("../../datasets/cifar-10-batches-bin/data_batch_2.bin");
   imDirs.push_back("../../datasets/cifar-10-batches-bin/data_batch_3.bin");
   imDirs.push_back("../../datasets/cifar-10-batches-bin/data_batch_4.bin");
   imDirs.push_back("../../datasets/cifar-10-batches-bin/data_batch_5.bin");
   imDirs.push_back("../../datasets/cifar-10-batches-bin/test_batch.bin");
   
   //load cifar10
   c.dataset.loadCifar10("../datasets/cifar-10-batches-bin/batches.meta.txt",imDirs);
   
  
   unsigned int nImages = (unsigned int) c.dataset.getSize();
   //cout << nImages << " images loaded.\n";
   
   //measure cpu time
   //cout << "Start timing\n";
   //time_t time0 = clock();
   
   c.constructCodebook();
   //cout << "Constructed codebooks in " << (double)(clock() - time0)/1000  << " ms\n";
   
   //c.exportCodebook("codebook10000HOG.bin");
   //c.importCodebook("maandag.bin");

   c.initSVMs();
   //cout << "Start training SVMs\n";
   //train convolutional SVMs
   //c.trainSVMs();
   
   //train classic SVM
   vector< vector< Feature> > trainActivations = c.trainClassicSVMs();
   
   //cout << "Testing on trainingsset:\n";
   //Testing phase
   unsigned int nCorrect = 0;
   unsigned int nFalse = 0;

   //for(size_t im = 0; im < 500 && im < nImages; ++im){
      //classify using convolutional SVMs
      //unsigned int result = c.classify(c.dataset.getImagePtr(im));
      //classify using classic SVMs
    //  unsigned int result = c.classifyClassicSVMs(c.dataset.getImagePtr(im), trainActivations, false /*im > 50200 - 0 - 10*/);
      //cout << "classifying image \t" << im << ": " << c.dataset.getImagePtr(im)->getLabel() << " is classified as " << c.dataset.getLabel(result) << endl;

    //  if((unsigned int)c.dataset.getImagePtr(im)->getLabelId() == result)
    //     ++nCorrect;
    //  else 
    //     ++nFalse;
      
   //}
  // cout << nCorrect << " correct, and " << nFalse << " false classifications, out of " << nCorrect + nFalse << " images\n";
   //cout << "Score: " << ((double)nCorrect * 100)/(nCorrect + nFalse) << "\% correct.\n";
   
   
   //*********************************************************************************************************************
   
   
  // cout << "Testing on Testset:\n";
   //Testing phase
   nCorrect = 0;
   nFalse = 0;

   for(size_t im = 50000-0; im < 50500; ++im){
      //classify using convolutional SVMs
      //unsigned int result = c.classify(c.dataset.getImagePtr(im));
      //classify using classic SVMs
      unsigned int result = c.classifyClassicSVMs(c.dataset.getImagePtr(im), trainActivations, false /*im > 50200 - 0 - 10*/);
      //cout << "classifying image \t" << im << ": " << c.dataset.getImagePtr(im)->getLabel() << " is classified as " << c.dataset.getLabel(result) << endl;

      if((unsigned int)c.dataset.getImagePtr(im)->getLabelId() == result)
         ++nCorrect;
      else 
         ++nFalse;
      
   }
   //cout << nCorrect << " correct, and " << nFalse << " false classifications, out of " << nCorrect + nFalse << " images\n";
   //cout << "Score: " << ((double)nCorrect*100)/(nCorrect + nFalse) << "\% correct.\n";
   
   return ((double)nCorrect)/(nCorrect + nFalse);
   
  // cout << "Processed in " << (double)(clock() - time0)/1000  << " ms\n";

   //return 0;
}






