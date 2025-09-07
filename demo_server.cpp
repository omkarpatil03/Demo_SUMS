#include <iostream>
#include <stdexcept>
#include <cppconn/driver.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/resultset.h>
#include <cppconn/prepared_statement.h>

using namespace std;


int main(void){

    try{
        //1. create driver and connection
        sql::Driver *driver;
        sql::Connection *conn;
        sql::PreparedStatement *pstmt;

        driver= get_driver_instance();

        conn= driver->connect("tcp://127.0.0.1:3306", "root", "root");

        //2. connect to the database

        conn->setSchema("demo_sums");

        //3. insert data

        pstmt= conn->prepareStatement("INSERT INTO demo_students(id, name, age) VALUES (?, ?, ?)");

        pstmt->setInt(1, 10);
        pstmt->setString(2, "OMKAR");
        pstmt->setInt(3, 24);
        pstmt->execute();

        cout<<"Data inserted successfully!"<<endl;

        delete pstmt;
        delete conn;

    }
    catch(sql::SQLException &e){
        cerr<<"Error:"<<e.what()<<endl;
    }

    return 0;
}