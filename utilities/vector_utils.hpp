#ifndef VECTOR_UTILS_HPP
#define VECTOR_UTILS_HPP

#include <vector>
#include <cmath>
#include <limits>
#include <string>

using namespace std;

// type of value that makes a vector
using COMPONENT = float;

class VectorUtils {

    public:

        // sums all of the elements in a vector
        static COMPONENT sum (vector<COMPONENT> v) {
            COMPONENT result = 0;
            for (auto i : v) {
                result += i;
            }
            return result;
        }

        // calculates the dot product of two vectors
        static COMPONENT dot_prod (vector<COMPONENT> v1, vector<COMPONENT> v2) {
            return sum(element_op(v1, v2, multiply));
        }

        // preform an operation between each of the elements in two vectors
        static vector<COMPONENT> element_op (vector<COMPONENT> v1, vector<COMPONENT> v2, COMPONENT (*operation)(COMPONENT, COMPONENT)) {
            if (v1.size() != v2.size()) return {};
            vector<COMPONENT> result;
            for (unsigned int i = 0; i < v1.size(); ++i) {
                result.push_back(operation(v1[i], v2[i]));
            }
            return result;
        }

        // preform an operation on each of the elements in a vector
        static vector<COMPONENT> element_dist (vector<COMPONENT> v, COMPONENT val, COMPONENT (*operation)(COMPONENT, COMPONENT)) {
            vector<COMPONENT> result;
            for (auto i : v) {
                result.push_back(operation(i, val));
            }
            return result;
        }

        // get vector length
        static COMPONENT length (vector<COMPONENT> v) {
            COMPONENT result = 0;
            for (auto i : v) {
                result += pow(i, 2);
            }
            return sqrt(result);
        }

        // get vector from two points
        static vector<COMPONENT> get_vect (vector<COMPONENT> p1, vector<COMPONENT> p2) {
            return element_op(p2, p1, subtract);
        }

        // get unit vector from vector
        static vector<COMPONENT> make_unit (vector<COMPONENT> v) {
            return element_dist(v, length(v), divide);
        }

        // get perpendicular vector
        static vector<COMPONENT> get_perp (vector<COMPONENT> v) {
            if (v.size() < 2 || v.size() > 3) return {};
            vector<COMPONENT> result;
            result.push_back(v[1] * -1);
            result.push_back(v[0]);
            if (v.size() == 3) result.push_back(v[2]);
            return result;
        }

        // get vector projection
        static vector<COMPONENT> get_proj (vector<COMPONENT> org, vector<COMPONENT> prj) {
            return element_dist(prj, dot_prod(org, prj), multiply);
        }

        // concatenate two vectors
        template <typename T>
        static vector<T> concat (vector<T> v1, vector<T> v2) {
            for (COMPONENT i : v2) {
                v1.push_back(i);
            }
            return v1;
        }

        template <typename T>
        static string get_string (vector<T> v, bool brackets) {
            string result = "";
            for (unsigned int i = 0; i < v.size(); ++i) {
                result += to_string(v[i]);
                if (i != v.size() - 1) {
                    result += " ";
                }
            }
            if (brackets) return "<" + result + ">";
            return result;
        }

        template <typename T>
        static string get_string (vector<T> v) {
            return get_string<T>(v, false);
        }

        // basic operations
        static COMPONENT add      (COMPONENT i, COMPONENT j) { return i + j; }
        static COMPONENT subtract (COMPONENT i, COMPONENT j) { return i - j; }
        static COMPONENT multiply (COMPONENT i, COMPONENT j) { return i * j; }
        static COMPONENT divide   (COMPONENT i, COMPONENT j) { return j != 0 ? i / j : numeric_limits<COMPONENT>::infinity(); }

    private:

        // constructor to prohibit creating instances
        VectorUtils () {}
};

#endif