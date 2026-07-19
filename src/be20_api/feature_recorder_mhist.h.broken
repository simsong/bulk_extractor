/**
 * histogram support.
 * We can ask the feature recorder to generate a histogram.
 * The file feature recorder re-reads the file.
 * The in-memory histogram feature recorder just records the features and outputs
 * them as a histogram when it shuts down (and whenever it runs out of memory!)
 * The SQL recorder uses an SQL query to make the histogram. So it never runs out of memory,
 * but it may run slow.
 */

/* in-memory histograms */
typedef atomic_histogram<std::string, uint64_t> mhistogram_t; // memory histogram
typedef std::map<histogram_def, mhistogram_t*> mhistograms_t;
