#include "tinyxml2.h"
#include <string>
#include <cstring>
#include <cctype>
#include <ctime>
#include <stdlib.h>
#include <windows.h>
#include <fstream>
using namespace std;

#define NUM_PRODUCTS_TO_CHECK_FOR 9
const char* badProductID[] = {"12BW", "fg", "01HS", "01PP", "01RI", "06HS", "02SS", "02DD", "19PS01"};
bool found_hold = false;



//Virtual Functions
void setHold(tinyxml2::XMLNode *Node);

bool startsWith(const char* base, char* sequence) {
    if(strlen(sequence) > strlen(base))
        return false;

    for(int i = 0; i < strlen(sequence); i++) {
        if(base[i] != sequence[i])
            return false;
    }
    return true;
}

char* getTime(const struct tm *timeptr) {
  static char result[26];
  sprintf(result, "%.2d:%.2d",
    timeptr->tm_hour, timeptr->tm_min);
  return result;
}

char* myAsctime(const struct tm *timeptr) {
  static const char mon_name[][4] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  static char result[26];
  sprintf(result, "%.3s%2d-%.2d.%.2d-%d",
    mon_name[timeptr->tm_mon],
    timeptr->tm_mday, timeptr->tm_hour,
    timeptr->tm_min,
    1900 + timeptr->tm_year);
  return result;
}

char* trim(const char *input) {
    int i,j;
    char *output = (char*)malloc(strlen(input));
    for (i = 0, j = 0; i < strlen(input); i++,j++)
    {
        if (!isspace(input[i]))
            output[j] = input[i];
        else
            j--;
    }
    output[j] = '\0';
    return output;
}

char* toUpperString(const char *input) {
    int i = 0;
    char *output = (char*)malloc(strlen(input));


    while(i < strlen(input)) {
        output[i] = toupper(input[i]);
        i++;
    }
    output[i-1] = '\0';
    return output;
}

bool check_hold(tinyxml2::XMLNode *Node) {
    tinyxml2::XMLNode *tempNode;
    const char* tempString;
    //Check the products for badProducts
    for(int a = 0; a < NUM_PRODUCTS_TO_CHECK_FOR; a++) {
        const char* prodID = badProductID[a];
        tempNode = Node->FirstChildElement("product01");

        for(int i = 0; i < 5; i++) {
            tempString = tempNode->ToElement()->GetText();

            if(!tempString)
                break;

            if(i > 1 && (startsWith(tempString, "10") || strstr(tempString, "04SW") ||
                         strstr(tempString, "04SF") || strstr(tempString, "04AM"))) {
                setHold(Node);
                printf("Multiple Product Confliction.\n");
                found_hold = true;
                return true;
            }

            if(tempString && strstr(tempString, prodID)) {
                setHold(Node);
                printf("Flagged Sku.\n");
                found_hold = true;
                return true;
            }
            tempNode = tempNode->NextSibling();
            tempNode = tempNode->NextSibling();
        }
    }

    //Check Shipping Information
    bool isInternational = strcmp(Node->FirstChildElement("country")->GetText(), "001") == 0 ? false : true;

    char *shipMethod = (char*)Node->FirstChildElement("shipvia")->GetText();
    if(!shipMethod) {
        setHold(Node);
        printf("Shipping conflict.\n");
        return true;
    }
    memcpy(shipMethod, (const char*)shipMethod, strlen(shipMethod));

    char *address = (char*)Node->FirstChildElement("saddress1")->GetText();
    memcpy(address, (const char*)address, strlen(address));


    address = toUpperString(trim((const char*) address));

    if(isInternational && strcmp(shipMethod, "PM") != 0) {
        setHold(Node);
        printf("Shipping confliction. \n");
        found_hold = true;
        return true;
    }
    if(strstr(address, "POBOX") && (strcmp(shipMethod, "1GD") == 0 ||
    strcmp(shipMethod, "FES") == 0 || strcmp(shipMethod, "FE2") == 0)) {
        setHold(Node);
        printf("Shipping confliction. \n");
        found_hold = true;
        return true;
    }
    if(strcmp(shipMethod, "") == 0) {
        setHold(Node);
        found_hold = true;
        return true;
    }

    return false;
}

void setHold(tinyxml2::XMLNode *Node){
    int year, month, day;
    char holdDate[20];
    char* workString;

    //get the order date element and get the text.
    tinyxml2::XMLNode *tempNode = Node->FirstChildElement("order_date");
    const char* tempString = tempNode->ToElement()->GetText();

    //transfer it to a non-const string
    workString = (char*)memcpy(holdDate, tempString, strlen(tempString));

    //get the necessary date
    year = atoi((const char*)strtok(workString, "- ")) + 1;
    month = atoi((const char*)strtok(NULL, "- "));
    day = 1;

    //transfer that date to another string.
    sprintf(holdDate, "%d-%d-%d\n", year, month, day);

    //Save it to the file.
    tempNode = Node->FirstChildElement("holddate");
    tempNode->ToElement()->SetText((const char*)holdDate);

}

void scanFile(tinyxml2::XMLDocument *myDoc) {
    tinyxml2::XMLNode *myNode = myDoc->FirstChild()->FirstChild();


    while(myNode!= NULL) {
        if(check_hold(myNode)) {
            printf("Hold placed on order %s.\n", myNode->FirstChildElement("odr_num")->GetText());
        }
        myNode = myNode->NextSibling();
    }

}

bool fexists(const char *filename)
{
  ifstream ifile(filename);
  return ifile;
}

int main() {
    time_t rawtime;
    struct tm* timeinfo;
    tinyxml2::XMLDocument myDoc;
    char filename[200];

    while(true) {

        time(&rawtime);
        timeinfo = localtime(&rawtime);
        if(true) {
            printf("Script Starting at %s\n\n", getTime(timeinfo));

            printf("Running Download....\n");
            system("bizsyncxlc.exe.lnk");

            if(fexists("orders.xml")) {
                myDoc.LoadFile("orders.xml");

                sprintf(filename, "BackupOrders\\%s.xml", myAsctime(timeinfo));
                printf("Backing up order....\n");
                myDoc.SaveFile(filename);

                printf("\nScanning for orders to put on hold....\n");
                scanFile(&myDoc);
                myDoc.SaveFile("Imports\\orders.xml", true);

                system("del orders.xml");
            }
            else
                printf("No new orders.");

            time(&rawtime);
            printf("\nScript finished successfully at %s\n", getTime(localtime(&rawtime)));
            printf("---------------------------------------\n");
            Sleep(60000);
        }
    }

    system("pause");
}
