
//  $Id: lab1_dtw.C,v 1.10 2009/09/18 02:12:13 stanchen Exp $

#include <cassert>
#include "util.H"

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

double EuclideanDistance(const matrix<double>& A, int idx1,
                         const matrix<double>& B, int idx2) {
  assert(A.size2() == B.size2());
  double dist = 0.0;
  for (size_t i = 0; i < A.size2(); ++i) {
    dist += pow(A(idx1, i) - B(idx2, i), 2);
  }
  return sqrt(dist);
}

double ComputeDistance(const matrix<double>& mat_hyp,
                       const matrix<double>& mat_templ) {
  double dist = 0.0;

  //  BEGIN_LAB
  //
  //  Input:
  //      "matHyp", matrix containing feature vectors for the current
  //      test utterance, and "matTempl", matrix containing feature
  //      vectors for the current template utterance being compared
  //      with.  These two matrices are guaranteed to have the same
  //      number of columns (but not the same number of frames).
  //
  //      For a matrix "mat", "mat.size1()" returns the number of
  //      frames, "mat.size2()" returns the number of dimensions in
  //      each feature vector, and "mat(frmIdx, dimIdx)" is the
  //      syntax for accessing elements.  Indices are numbered from 0.
  //
  //  Output:
  //      Set "dist" to the total distance between these two utterances.
  //
  //  P=1, r=100
  int I = mat_hyp.size1();
  int J = mat_templ.size1();
  matrix<double> g;
  g.resize(I+1, J+1);
  g.clear();

  // initial value
  double N = I+J;
  int r = 100;
  int i=1, j=1;
  g(0 ,0) = 0;
  g(1, 1) = 2*EuclideanDistance(mat_hyp, 0, mat_templ, 0);

  while(1){
      i += 1;
      if(i>j+r){
          j += 1;
          if(j > J){
              dist = g(I, J) / N;
              break;
          }else{
              i = j - r - 1;
          }
      }else if(i < 1 || i > I){
          continue;
      }else{
          //cout << "i=" << i << ", j=" << j << endl;
          double dist_i_j = EuclideanDistance(mat_hyp, i-1, mat_templ, j-1);
          double dist_i_j1 = DBL_MAX;
          double dist_i1_j = DBL_MAX;
          if(j-2>0){
            dist_i_j1 = EuclideanDistance(mat_hyp, i-1, mat_templ, j-2);
          }
          if(i-2>0){
            dist_i1_j = EuclideanDistance(mat_hyp, i-2, mat_templ, j-1);
          }
          
          // min1, min2, min3
          double min1 = 0;
          if(j-2 < 0){
              min1 = 2*dist_i_j1 + dist_i_j;
          }else{
              min1 = g(i-1, j-2) + 2*dist_i_j1 + dist_i_j;
          }
          double min2 = g(i-1, j-1) + 2*dist_i_j;
          double min3 = 0;
          if(i-2<0){
              min3 = 2*dist_i1_j + dist_i_j;
          }else{
              min3 = g(i-2, j-1) + 2*dist_i1_j + dist_i_j;
          }

          g(i, j) = min(min(min1, min2), min3);

      }
  }

  //  END_LAB
  return dist;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void MainLoop(const char** argv) {
  //  Process command line arguments.
  map<string, string> params;
  process_cmd_line(argv, params);
  bool verbose = get_bool_param(params, "verbose");

  //  Load feature files for templates.
  //  Get template label from matrix name.
  vector<string> templateLabelList;
  vector<matrix<double> > templateMatList;
  ifstream templateStrm(
      get_required_string_param(params, "template_file").c_str());
  while (templateStrm.peek() != EOF) {
    templateMatList.push_back(matrix<double>());
    string labelStr = read_float_matrix(templateStrm, templateMatList.back());
    templateLabelList.push_back(labelStr);
  }
  templateStrm.close();
  if (templateMatList.empty()) throw runtime_error("No templates supplied.");

  //  Load correct label for each feature file, if present.
  vector<string> featLabelList;
  string labelFile = get_string_param(params, "feat_label_list");
  if (!labelFile.empty()) read_string_list(labelFile, featLabelList);

  //  The main loop.
  ifstream featStrm(get_required_string_param(params, "feat_file").c_str());
  matrix<double> feats;
  unsigned templCnt = templateLabelList.size();
  unsigned uttCnt = 0;
  unsigned correctCnt = 0;
  while (featStrm.peek() != EOF) {
    int uttIdx = uttCnt++;
    string idStr = read_float_matrix(featStrm, feats);

    //  Find closest template.
    int bestTempl = -1;
    double bestScore = DBL_MAX;
    for (unsigned templIdx = 0; templIdx < templCnt; ++templIdx) {
      if (feats.size2() != templateMatList[templIdx].size2())
        throw runtime_error("Mismatch in test/template feature dim.");
      double curScore = ComputeDistance(feats, templateMatList[templIdx]);
      if (verbose)
        cout << format("  %s: %.3f") % templateLabelList[templIdx] % curScore
             << endl;
      if (curScore < bestScore) {
        bestScore = curScore;
        bestTempl = templIdx;
      }
    }
    if (bestTempl < 0) throw runtime_error("No alignments found in DTW.");

    string hypLabel = (bestTempl >= 0) ? templateLabelList[bestTempl] : "";
    if (!featLabelList.empty()) {
      //  If have reference labels, print ref and hyp classes.
      if (uttIdx >= (int)featLabelList.size())
        throw runtime_error(
            "Mismatch in number of utterances "
            "and labels.");
      string refLabel = featLabelList[uttIdx];
      cout << format("Reference: %s, Hyp: %s, Correct: %d") % refLabel %
                  hypLabel % (hypLabel == refLabel)
           << endl;
      if (hypLabel == refLabel) ++correctCnt;
    } else
      //  If don't have reference labels, just print hyp class.
      cout << hypLabel << " (" << idStr << ")" << endl;
  }
  featStrm.close();
  if (!featLabelList.empty()) {
    //  If have reference labels, print accuracy.
    unsigned errCnt = uttCnt - correctCnt;
    cout << format("Accuracy: %.2f%% (%d/%d), Error rate: %.2f%% (%d/%d)") %
                (100.0 * correctCnt / uttCnt) % correctCnt % uttCnt %
                (100.0 * errCnt / uttCnt) % errCnt % uttCnt
         << endl;
  }
}

int main(int argc, const char** argv) {
  try {
    MainLoop(argv);
  } catch (exception& xc) {
    cerr << "Error: " << xc.what() << endl;
    return -1;
  }
  return 0;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
