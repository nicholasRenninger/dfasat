"""
This is an estimator based on the DFASAT library developed at the University of Luxembourg
"""
import numpy as np
import scipy as sp
import numbers
from sklearn.base import BaseEstimator
import flexfringe.lib.flexfringe as flexfringe
import flexfringe.eval as e
import sys
from multiprocessing import Process, cpu_count
import random
import signal
signal.signal(signal.SIGINT, signal.SIG_DFL)

import os,sys,select

# output capture based on
# https://stackoverflow.com/a/9489139
def more_data(elm):
    r, _, _ = select.select([elm],[],[],0)
    return bool(r)

def read_pipe(elm):
    out = ''
    while more_data(elm):
        out += " " + str(os.read(elm, 1024))
    return out

has_graphviz = True
try:
    from graphviz import Source
except:
    has_graphviz = False


class StateMachinePlot():
    """ A helper class implementing a _repr_svg_ method

    Parameters
    ----------
    svg : string
        The plot in SVG format
    """
    _svg = None

    def __init__(self, svg):
        self._svg = svg

    def _repr_svg_(self):
        return self._svg


num_cores = cpu_count()

class BaggingClassifier:
    estimator = None
    number = 10
    estimators = []
    file_prefix = "dfa"

    def __init__(self, estimator=estimator, number=10, output_file="dfa", random_seed=False, random_counts=None, **kwargs):
        self.estimator = estimator
        self.number = number
        self.file_prefix = output_file

        for _ in range(number):
            if random_seed:
                kwargs['seed'] = random.randint(0, 5000000)
            if random_counts is not None:
                kwargs['state_count'] = random.choice(random_counts)
                kwargs['symbol_count'] = random.choice(random_counts)

            self.estimators.append(estimator(**kwargs))

    def calc_model(self, i, estimator, X, y):
        estimator.fit(X, y)
        with open(self.file_prefix + '_' + str(i) + '.model.dot', 'w') as r:
            r.write(estimator.merger.dot_output)


    def fit(self, X, y, subset=True):
        processes = []
        for i, estimator in enumerate(self.estimators):
            if subset:
                print('randomly sampling')
                s_X=[]
                for x in X:
                    if random.randint(1, 5) != 1:
                        s_X.append(x)
                s_y = [1]*len(s_X)
            else:
                s_X = X
                s_y = y

            p = Process(target=self.calc_model, args=(i, estimator, s_X, s_y))
            processes.append(p)
            print('starting process '+str(i))
            p.start()
            if (i+1) % num_cores == 0:
                for p in processes:
                    p.join()
                    processes.remove(p)
        for p in processes:
            p.join()
            processes.remove(p)


    def predict(self, prefix):
        predictions = []
        for estimator in self.estimators:
            m = e.load_model(estimator.merger.dot_output, normalize=True)
            prediction = e.predict(prefix, m)
            predictions.append(prediction)

        sorted_predictions = {}
        for sample in predictions:
            for p in sample:
                if p[0] not in sorted_predictions:
                    sorted_predictions[p[0]] = p[1][1]
                else:
                    sorted_predictions[p[0]] += p[1][1]

        sorted_predictions = sorted(sorted_predictions.items(), key=lambda p: p[1], reverse=True)

        return sorted_predictions

class DFASATEstimator(BaseEstimator):
    """ An estimator that uses the DFASAT library

       import code
       code.interact(local=locals())

    """
    result_dot = None
    merger = None
    parameters = None
    output_file = "dfa"
    output_counter = 1

    def __init__(self, **kwargs):
        """hData="likelihood_data", hName="likelihoodratio",
            tries=1, sat_program="", dot_file="dfa", sinkson=1, seed=12345678,
            apta_bound=2000, dfa_bound=50, merge_sinks_p=1, merge_sinks_d=0,
            lower_bound=-1.0, offset=5, method=1, heuristic=1, extend=1, symbol_count=10,
            state_count=25, correction=1.0, parameters=0.5, extra_states=0,
            target_rejecting=0, symmetry=1, forcing=0"""

        self.parameters = flexfringe.parameters()
        for key, value in kwargs.items():
            if key == 'output_file':
                self.output_file = value
                continue
            if not hasattr(self.parameters, key):
                raise Exception(key + " not a valid argument")
            setattr(self.parameters, key, value)
        """p.hData = hData
        p.hName = hName
        p.tries = tries
        p.sat_program = sat_program
        p.dot_file = dot_file
        p.sinkson = sinkson
        p.seed = seed
        p.apta_bound = apta_bound
        p.dfa_bound = dfa_bound
        p.merge_sinks_p = merge_sinks_p
        p.merge_sinks_d = merge_sinks_d
        p.lower_bound = lower_bound
        p.offset = offset
        p.method = method
        p.heuristic = heuristic
        p.extend = extend
        p.symbol_count = symbol_count
        p.state_count = state_count
        p.correction = correction
        p.parameters = parameters
        p.extra_states = extra_states"""


    def _verify_x(self, arr):
        if isinstance(arr, np.ndarray):
            x = arr
            arr = arr.flatten().tolist()
            if len(arr) == 0:
                raise ValueError("0 feature(s) (shape=" + str(x.shape) + ") while a minimum of " + str(x.shape[0]) + " is required.")
        if sp.sparse.issparse(arr):
            raise TypeError("sparse matrices not supported")
        for n in arr:
            if not isinstance(n, numbers.Number):
                pass
#                print(n)
#                raise TypeError("argument must be a string or a number")
#            if np.isinf(n) or np.isnan(n):
#                raise ValueError("argument cannot contain info or NaN")

    def _verify_y(self, arr):
        pass

    def _verify_args(self, x, y):
        if isinstance(x, np.ndarray) and isinstance(y, np.ndarray):
            if x.shape[0] != y.shape[0]:
                raise ValueError("X and y must have the same number of samples")
        self._verify_x(x)
        self._verify_y(y)

    def _header(self, X):
        return str(len(X)) + " " + str(self._alphabet_size(X))

    def _alphabet_size(self, X):
        s = set()
        for row in X:
            for value in row[1:]:
                s.add(value.split("/")[0])
        print(s)
        return len(s)

    def _numpy_to_str(self, x):
        return [" ".join([str(int(symbol)) for symbol in sample]) for sample in x]

    def get_params(self, deep):
        params = {}
        for pName in dir(self.parameters):
            if not pName.startswith("__"):
                params[pName] = getattr(self.parameters, pName)
        return params

    def set_params(self, params):
        for pName, val in params.items():
            setattr(self.parameters, pName, val)


    def fit(self, x, y):
        # merge x and y into our format (array of strings)
        if type(x) == np.ndarray:
            x = self._numpy_to_str(x)
        self._verify_args(x, y)

        header = self._header(x)
        transformed_x = [" ".join([str(value) for value in row]) for row in x]
        for i in range(len(y)):
           transformed_x[i] = str(y[i]) + " " + transformed_x[i]

        transformed_x.insert(0, header)

        output_file = self.output_file + str(self.output_counter)
        self.output_counter += 1

        self.fit_(dfa_data=transformed_x, output_file=output_file)
        return self

    def fit_(self, dfa_file=None, dfa_data=None, output_file="dfa"):
        """A reference implementation of a fitting function

        Parameters
        ----------
        hData : string
            name of the evaluation data class
        hName : string
            name of the evaluation function
        tries : integer
            number of iterations
        sat_program : string
            path to the sat solver program
        dfa_file : string
            path to the dfa file
        dfa_data : list of strings
            dfa data as array of strings

        Returns
        -------
        self : object
            Returns self.
        """
        self.result_dot = None
        if dfa_file:
            print("dfa file set to " + dfa_file)
            self.parameters.dfa_file = dfa_file
        elif dfa_data:
            print("dfa_data set")
            self.parameters.dfa_data = flexfringe.vector_less__std_scope_string__greater_()
            for s in dfa_data:
                self.parameters.dfa_data.append(s+"\n")
        else:
            raise TypeError('One of dfa_file or dfa_data has to be given')

        flexfringe.init_with_params(self.parameters)
        a = flexfringe.apta()
        l = getattr(flexfringe, self.parameters.hName)()

        self.merger = flexfringe.state_merger(l, a)
        if self.parameters.dfa_file:
            self.merger.read_apta(self.parameters.dfa_file)
        else:
            self.merger.read_apta(self.parameters.dfa_data)

        output_dot_file = output_file + ".dot"
        output_aut_file = output_file + ".aut"
        print('starting flexfringe')
        sys.stdout.write('\b')
        pipe_out, pipe_in = os.pipe()
        stdout = os.dup(1)
        os.dup2(pipe_in, 1)
        flexfringe.dfasat(self.merger, self.parameters.sat_program, output_dot_file, output_aut_file)
        print(read_pipe(pipe_out).replace('\\n', " | ").replace(" b\'", ""))
        os.dup2(stdout,1)
        print('flexfringe done')
        self.result_dot = output_dot_file
        return self

    def predict(self, X):
#        self._verify_x(X)
        """ A reference implementation of a predicting function.

        Parameters
        ----------
        X : array-like of shape = [n_samples, n_features]
            The input samples.

        Returns
        -------
        y : array of shape = [n_samples]
            Returns :math:`x^2` where :math:`x` is the first column of `X`.
        """
        if not self.result_dot:
            raise Exception("Run the fit function first!")

        model = e.load_model(self.result_dot)

        return e.predict(X, model)

    def plot(self):
        """ Plot the state machine after fitting

        Returns
        -------
        A StateMachinePlot object implementing a _repr_svg_ method, used
        by Jupyter/IPython notebook to plot a graph
        """
        if has_graphviz:
            if self.result_dot is not None:
                with open("pre_"+self.result_dot) as result_dot_file:
                    return StateMachinePlot(
                       Source(result_dot_file.read())._repr_svg_()
                    )
            else:
                print("Run fit first!")
        else:
            print("This feature requires the graphviz python library")
