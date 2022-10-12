#include <stdio.h>
#include <iostream>
#include <vector>
#include <unistd.h> 
#include <sys/types.h>
#include <sys/wait.h>


using std::cout;
using std::endl;
using std::string;
using std::to_string;



// Name: Dao Van Ngoc Hoang
// std-ID: 1902093.



// CLASS
class Matrix {
    private:
        // ------ VARIABLES  ----------
        int** matrix;

        // ------ METHODS  ----------
        int** poppulate(int rows, int columns, string option);

    public:
        // ------ VARIABLES  ----------
        int rows, cols;


        // ------ CONSTRUCTOR----------
        Matrix(int inp_rows, int inp_cols ){
            rows = inp_rows;
            cols = inp_cols;

            matrix = this->poppulate(inp_rows, inp_cols, "rand");
        }


        // ------ METHODS  ----------
        // print matrix into human readable array 
        void print();
        void print(int rows, int cols, int** matrix);

        // get matrix 
        int ** get();

        // check compatiple 
        int check(Matrix inp_matrix);

        // operation using multi-processing
        int** matmul_parallel_rows(Matrix inp_matrix_object);

        // function level test
        int** matmul_parallel_v2(Matrix inp_matrix_object);

        // function level test
        int** matmul_parallel_element(Matrix inp_matrix_object);

        // serial matrix multiplication
        int ** matmul_serial (Matrix inp_matrix_object);

        // Row compute
        int* row_compute(int row_at, int out_cols, int sharedim, int** inp_matrix);
        
        // Row compute parallel        
        int* row_compute_parallel(int row_at, int out_cols, int sharedim, int** inp_matrix);


        // Element compute
        int element_compute(int row_at, int out_col_at, int sharedim, int** inp_matrix );
};




// ------ IMPLEMENTATION ----------

// print matrix into human readable array 
void Matrix::print(int rows, int cols, int** matrix){

    cout << "\n[" ;
    for (int r = 0; r < rows; r ++ ){
        // rows have to have lower level 
        if (r > 0){
            cout << " " ;
        }
        cout << "[" ;
        for (int c = 0; c < cols; c++){
            if (c == (rows -1)){
                cout << matrix[r][c];
            }else{
                cout << matrix[r][c]<< "," ;
            }
        }

        if (r == (rows -1)) 
            cout << "]";
        else 
            cout << "]" << endl;
    }
    cout << "] \n" << endl;
};



// retrurn matrix 
int **  Matrix::get(){
    return Matrix::matrix;
}


// checking method
int Matrix::check(Matrix inp_matrix_object){

    // check compatiple
    if (Matrix::cols != inp_matrix_object.rows) {
        printf("\n\n ERROR: ");
        printf("cannot multiply 2 arrays with shape (%d, %d) and (%d, %d) \n\n", Matrix::rows, Matrix::cols, inp_matrix_object.rows, inp_matrix_object.cols);
        return -1;
    }
    
    return 0;
}



// function level test
int** Matrix::matmul_parallel_element(Matrix inp_matrix_object) {

    
    // to store the execution time of code
    double time_spent = 0.0;
    clock_t begin = clock();
 
    int share_dim ;

    int output_rows = Matrix::rows, output_cols = inp_matrix_object.cols;
    
    // check compatiple
    if (Matrix::check(inp_matrix_object) == -1 )
        exit(0);

    // share_dim that stand for the matching cols and input rows.
    share_dim = inp_matrix_object.rows ;

    // initialize result matrix 
    int** result_matrix = Matrix::poppulate(Matrix::rows, share_dim, "any"); 


    // get the matrix from marix object.
    int** inp_matrix = inp_matrix_object.get();


    // start operation
    for (int i = 0 ; i < Matrix::rows; i ++) {

        // create a new array for writing the row result
        int* row_result = Matrix::row_compute_parallel(i, output_cols, share_dim, inp_matrix);

        // and put them into the result matrix
        for (int j = 0; j < output_cols; j ++){
                result_matrix[i][j] = row_result[j];
        }
        // we exit to make sure 1 child is create 1 time in the loop
        // and not create it's own childrens because of the loop.

    
        // free memory
        free(row_result);

        
    }


    // end clock
    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    cout << "\nparallel element time spend:  " <<time_spent << endl;
    cout << "number of child process = " << 1 + Matrix::cols<<endl;


    return result_matrix;

}


// function level test
int** Matrix::matmul_parallel_v2(Matrix inp_matrix_object) {

    
    // to store the execution time of code
    double time_spent = 0.0;
    clock_t begin = clock();
 
    int share_dim ;

    int output_rows = Matrix::rows, output_cols = inp_matrix_object.cols;
    
    // check compatiple
    if (Matrix::check(inp_matrix_object) == -1 )
        exit(0);

    // share_dim that stand for the matching cols and input rows.
    share_dim = inp_matrix_object.rows ;

    // initialize result matrix 
    int** result_matrix = Matrix::poppulate(Matrix::rows, share_dim, "any"); 


    // get the matrix from marix object.
    int** inp_matrix = inp_matrix_object.get();

    // create file descriptor for processes communication.
    int file_descriptor[Matrix::rows][2];



    // start operation
    for (int i = 0 ; i < Matrix::rows; i ++) {
        
        if (pipe(file_descriptor[i]) < 0){
            perror("pipe error");
        }

        int pid = fork();

        if (pid == -1){
            printf("error");
            exit(0);

        // child process is allowed only run this block
        }else if (pid == 0){

            // create a new array for writing the row result
            int* row_result = Matrix::row_compute_parallel(i, output_cols, share_dim, inp_matrix);
            
            // share value from the child processes to parent process using file descriptor.
            write(file_descriptor[i][1], row_result, sizeof(int)* Matrix::rows);
            close(file_descriptor[i][1]);

            // free memory
            free(row_result);

            // we exit to make sure 1 child is create 1 time in the loop
            // and not create it's own childrens because of the loop.
            exit(0);
            break;

        }
    }

    while ( wait(NULL) != -1 || errno != ECHILD); // wait for all child complete.
    
    
    // Parent will aggregate 
    for (int i = 0; i<Matrix::rows; i++){

        // read each row returned from the child process
        int result[Matrix::rows];
        read(file_descriptor[i][0], result, sizeof(int) * Matrix::rows);

        // close filde descriptor.
        close(file_descriptor[i][1]);
        close(file_descriptor[i][0]);

        // and put them into the result matrix
        for (int j = 0; j < output_cols; j ++){
            result_matrix[i][j] = result[j];
        }
    }

    // end clock
    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    cout << "\nparallel rows + elements level time spend:  " <<time_spent << endl;
    cout << "number of child process = " << Matrix::rows*Matrix::cols + 1<<endl;



    return result_matrix;

}



// operation using multi-processing
int** Matrix::matmul_parallel_rows(Matrix inp_matrix_object){

    // to store the execution time of code
    double time_spent = 0.0;
    clock_t begin = clock();
 
    int share_dim ;

    int output_rows = Matrix::rows, output_cols = inp_matrix_object.cols;
    
    // check compatiple
    if (Matrix::check(inp_matrix_object) == -1 )
        exit(0);

    // share_dim that stand for the matching cols and input rows.
    share_dim = inp_matrix_object.rows ;

    // initialize result matrix 
    int** result_matrix = Matrix::poppulate(Matrix::rows, share_dim, "any"); 


    // get the matrix from marix object.
    int** inp_matrix = inp_matrix_object.get();

    
    int pid_list[Matrix::rows];
    int file_descriptor[Matrix::rows][2];



    // start operation
    for (int i = 0 ; i < Matrix::rows; i ++) {
        
        if (pipe(file_descriptor[i]) < 0){
            perror("pipe error");
        }

        pid_list[i] = fork();

        if (pid_list[i] == -1){
            printf("error");
            exit(0);

        // child process is allowed only run this block
        }else if (pid_list[i] == 0){
            
            // close file des
            close(file_descriptor[i][0]);

            // create a new array for writing the row result
            int* row_result = Matrix::row_compute(i, output_cols, share_dim, inp_matrix);

            // share value from the child processes to parent process using file descriptor.
            write(file_descriptor[i][1], row_result, sizeof(int)* Matrix::rows);
            close(file_descriptor[i][1]);

            // free memmory
            free(row_result);
            exit(0);
            break;

        }
    }

    while ( wait(NULL) != -1 || errno != ECHILD); // wait for all child complete.
    // Parent will aggregate 
    
    for (int i = 0; i<Matrix::rows; i++){

        close(file_descriptor[i][1]);

        // read each row returned from the child process
        int result[Matrix::rows];
        read(file_descriptor[i][0], result, sizeof(int) * Matrix::rows);

        // and put them into the result matrix
        for (int j = 0; j < output_cols; j ++){
            result_matrix[i][j] = result[j];
        }
    }

    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    cout << "\nparallel row level time spend:  " <<time_spent << endl;
    cout << "number of child process = " << Matrix::rows + 1<<endl;

    return result_matrix;

}



// serial matrix multiplication
int** Matrix::matmul_serial(Matrix inp_matrix_object){
    
    // to store the execution time of code
    double time_spent = 0.0;
    clock_t begin = clock();

    int inp_cols = inp_matrix_object.cols , share_dim ;
    
    // check compatiple
    if (Matrix::check(inp_matrix_object) == -1 )
        exit(0);
    
    // share_dim that stand for the matching cols and input rows.
    share_dim = inp_matrix_object.rows ;

    // initialize result matrix 
    int** result_matrix = Matrix::poppulate(Matrix::rows, share_dim, "any"); 

    // get the matrix from marix object.
    int** inp_matrix = inp_matrix_object.get();

    // start operation
    for (int i = 0 ; i < Matrix::rows; i ++) {

        for (int j = 0 ; j < inp_cols; j++){
            result_matrix[i][j] = 0;

            for (int k = 0; k < share_dim ; k ++) {
                result_matrix[i][j] += Matrix::matrix[i][k] * inp_matrix[k][j];
            }
        }
    }
    

    clock_t end = clock();
    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    cout << "\nsequential opetation time spend:  " <<time_spent << endl;
    return result_matrix;

};



// Row compute parallel
int* Matrix::row_compute_parallel(int row_at, int out_cols, int sharedim, int** inp_matrix){
    
    // the array must be allocated in heap.
    int *row_values = (int*) malloc(sizeof(int)*Matrix::rows);

    int file_descriptor[out_cols][2];


    for (int j = 0 ; j < out_cols; j++){

        if (pipe(file_descriptor[j]) < 0){
            perror("pipe error");
        }
        int pid = fork();
        if (pid == -1){
                printf("error");
                exit(0);
        }
        if (pid == 0){

            // child process
            close(file_descriptor[j][0]);

            // calculate the elements.
            int element_value = element_compute(row_at, j, sharedim, inp_matrix); 

            // write to parent.
            write(file_descriptor[j][1], &element_value, sizeof(int));
            close(file_descriptor[j][1]);
            exit(0);
            break;
        }
    }

    while ( wait(NULL) != -1 || errno != ECHILD); // wait for all child complete.
    
    for (int i = 0; i < out_cols; i++){
        
        // close file descriptor
        close(file_descriptor[i][1]);


        // read each row returned from the child process
        int result ;
        read(file_descriptor[i][0], &result, sizeof(int));

        // put them in to the array to return
        row_values[i] = result;
        close(file_descriptor[i][0]);


    }

    return row_values;
    
};




// Row compute
int* Matrix::row_compute(int row_at, int out_cols, int sharedim, int** inp_matrix){
    
    int *row_values = (int*) malloc(sizeof(int)*Matrix::rows);



    for (int j = 0 ; j < out_cols; j++){


            // calculate the elements.
            int element_value = element_compute(row_at, j, sharedim, inp_matrix); 
            // put them in to the array to return
            row_values[j] = element_value;
            
    }

    return row_values;
    
};



// Element compute
int Matrix::element_compute(int row_at, int out_col_at, int sharedim, int** inp_matrix) {

    int element_value = 0;
    for (int k = 0; k < sharedim; k++){
        element_value += Matrix::matrix[row_at][k] * inp_matrix[k][out_col_at];
    }
    return element_value;
};  



// Generate Matrix
int** Matrix::poppulate(int rows, int columns, string option = "rand"){

    // initialize array point to the pointer array that is an array of pointer with the size of rows
    int** matrix ;
    matrix = new int*[rows];

    // for each rows
    for (int row = 0; row < rows;  row++) {
        // initialize the array that contain the actual values of the rows with size of columns
        matrix[row] = new int[columns];

        if (option == "rand")
            // for each column
            for (int col = 0; col < columns;  col++) {
                // assign a random value in to every position by using row, col as index.
                matrix[row][col] = rand() % 100 / 10;   
            }     
    }   
    return matrix;
}



void Matrix::print(){
    Matrix::print(Matrix::rows, Matrix::cols, Matrix::matrix);
}






int main (){


    int r = 1000, c = 1000;

    Matrix matrix_1 = Matrix(r, c);
    Matrix matrix_2 = Matrix(r, c);

    printf("Multiply 2 matrix with shape of (%d, %d) x (%d, %d)", matrix_1.rows, matrix_1.cols, matrix_2.rows, matrix_2.cols);



    // matrix_1.print();
    // cout << "X" << endl;
    // matrix_2.print();
    // cout << "result: \n" << endl;



    // int** result_1 = matrix_1.matmul_serial(matrix_2);
    int** result_2 = matrix_1.matmul_parallel_rows(matrix_2);
    // int** result_3 = matrix_1.matmul_parallel_v2(matrix_2);
    // int** result_4 = matrix_1.matmul_parallel_element(matrix_2);


    // matrix_1.print(r, c, result_1);
    // matrix_1.print(r, c, result_2);
    // matrix_1.print(r, c, result_3);
    // matrix_1.print(r, c, result_4);



    return 0;
}


