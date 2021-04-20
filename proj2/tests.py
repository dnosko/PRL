import numpy as np
import os
import subprocess
import re

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

class Test:
    max_processors = 20
    program_name = "hello"#"mm"
    program_ext = ".cpp"
    path = os.path.dirname(os.path.abspath(__file__))
    passed = False

    def __init__(self, mat1, mat2, test_name):
        
        self.mat1 = np.array(mat1)
        self.mat2 = np.array(mat2)
        self.name = test_name
        self.product = np.matmul(mat1,mat2)
        print(self.product)
        self.__check_processors()
        
    def __create_files(self):
        
        mat1_f = open(os.path.join(self.path, "mat1"), "w")
        self.write_to_file(self.rows, self.mat1, mat1_f)
        mat2_f = open(os.path.join(self.path, "mat2"), "w")
        self.write_to_file(self.cols, self.mat2, mat2_f)

    def write_to_file(self, count, matrix, file):
        np.savetxt(file, matrix,fmt="%d", header=str(count), comments='')
        file.close()

    def __check_processors(self):
        self.rows = len(self.mat1)
        self.cols = len(self.mat2[0])
        self.processors = self.rows*self.cols
        try:
            assert self.processors <= self.max_processors
        except AssertionError:
            print(f"{bcolors.WARNING}Warning: Prekroceny max pocet procesorov{bcolors.ENDC}")
            print(self.rows, self.cols, " = ",self.processors)

    def run(self):
        # bla bla do multiplictation and write result
        self.__create_files()
        #args = "mpirun -np " + self.processors + self.program_name
        args = "./hello"
        self.run_cpp(args, self.program_name+self.program_ext)
        self.compare_output()
        #compare
        if (self.passed):
            print(f"{bcolors.OKGREEN}Passed " +self.name +f"{bcolors.ENDC}")
            return True
        else:
            print(f"{bcolors.FAIL}Failed " +self.name +f"{bcolors.ENDC}")
            return False
        
    def run_cpp(self, args, filename):
        proc = subprocess.Popen([os.path.join(self.path,args), os.path.join(self.path,filename)], stdout=subprocess.PIPE)
        self.output = proc.stdout.read()
        proc.wait()

    def read_output(self, output):
        output = output.decode('ascii')
        splitted = output.split('\n', maxsplit=1)
        rows, cols = splitted[0].split(':')
        rows = int(rows)
        cols = int(cols)

        # replace spaces and new line into comma 
        mat = re.split(' |\n',splitted[1])

        # if the last element is \n or white space
        if mat[-1] == '' or mat[-1] == "\n":
            mat = mat[:-1]

        mat = np.array(mat)
        # reshape to matrix
        matrix = mat.reshape(rows, cols)
        matrix = matrix.astype(np.int)

        return rows, cols, matrix
    
    def compare_output(self):
        r, c, matrix = self.read_output(self.output)

        if r == self.rows and c == self.cols and np.all(matrix == self.product):
            self.passed = True
        else:
            self.passed = False

    def destruct_files(self):
        os.remove("mat1")
        os.remove("mat2")


class Tests:

    def __init__(self, tests):
        self.tests = tests
        self.all_tests = len(tests)
        self.passed = 0
        pass

    def run_tests(self):
        for test in self.tests:
                if test.run():
                    self.passed += 1
        
        output_str = "Passed " + str(self.passed) + "/" + str(self.all_tests)
        print(print(f"{bcolors.OKCYAN}" + output_str + f"{bcolors.ENDC}"))
                    

if __name__ == "__main__":
    tests = Tests([
        Test([[1,-1],[2, 2],[3, 3]], [[1, -2, -2, -8], [-1, -2, 7, 10]], "test1"), 
        Test([[1, 0],[0, 1]],[[4, 1],[2, 2]], "test2")
        ])
    tests.run_tests()