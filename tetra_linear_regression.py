from sklearn import linear_model
import numpy as np
import pdb
import sys, traceback


# note: the original code in this file can from Danny Tarlow, showing how 
# sklearn could be used to perform L1 regression given a high-level problem 
# statement of mapping f(a,b,c) => latency 

def make_dans_email_data(augment=False):
    """ 
    returns a tuple of 
    1. raw_X: unprocessed inputs.
    2. y: the target outputs

    No feature engineering is done yet
    """
    raw_X = [
        (1, 1, 1) ,
        (3, 2, 10),
        (6, 100, 10000),
        ]

    y = [2, 11, 136]

    if augment:
        # resolve confusion about a**2 vs a**3
        raw_X.append((10, 1, 23))
        y.append(101)

    return raw_X, y

# currently, assume there is only a single method + URL combo
# and that all params are 3 integers, a, b, and c.  
def read_tetra_data_file(fname):
    raw_X = []
    y = [] 
    with open(fname) as f:
        for line in f.readlines():
            vals = line.split(",")
            param_str_list = vals[2].split("&")
            a = int(param_str_list[0].split("=")[1])
            b = int(param_str_list[1].split("=")[1])
            c = int(param_str_list[2].split("=")[1])
            raw_X.append((a,b,c))
            y.append(int(vals[3])) 
    print("raw_X = %s" % raw_X)
    print("y = %s" % y)
    return raw_X, y 

def make_polynomial_features(raw_X):
    """ 
    Engineer features so that the output can be predicted as 
    a linear combination of features. Here, given a raw input of
    (a, b, c), we'll create a feature vector of 
    (1, a, a**2, a**3, b, b**2, b**3, c, c**2, c**3).
    
    Later, this should be expanded to include cross terms like ab, a**2 b, etc.
    """
    X = []
    feature_names = []
    for raw_x in raw_X:
        a, b, c, = raw_x
        x = (1, a, a**2, a**3, b, b**2, b**3, c, c**2, c**3)
        X.append(x)

    feature_names = ("1",
                     "a", "a**2", "a**3",
                     "b", "b**2", "b**3",
                     "c", "c**2", "c**3")

    return np.array(X), feature_names


def train_linear_model(X, y, regularization=None):
    """
    Create and learn a linear regression model. Return learned parameter vector.
    """
    if regularization is None:
        model = linear_model.LinearRegression(fit_intercept=False)
    elif regularization == "L1":
        model = linear_model.Lasso(fit_intercept=False)
    model.fit(X, y)
    return model


def print_learned_model(model, feature_names):
    coefficients = model.coef_
    for i, name in enumerate(feature_names):
        print("%s\t%s" % (name, np.round(coefficients[i], decimals=2)))


def main():

    raw_X, y = read_tetra_data_file("tetra-data.txt")

    # Create some data and engineer some polynomial features.
    # It didn't quite work with the dataset given. Set augment=True
    # to get one more data point that will make it work.
    #raw_X, y = make_dans_email_data(augment=True)
    
    X, feature_names = make_polynomial_features(raw_X)
    y = np.array(y)

    # Use linear regression, but regularization will be important
    # if there isn't much data.
    model = train_linear_model(X, y, "L1")
    print_learned_model(model, feature_names)


if __name__ == "__main__":
    try:
        main()
    except:
        type, value, tb = sys.exc_info()
        traceback.print_exc()
        pdb.post_mortem(tb)
