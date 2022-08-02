#include <iostream>
#include <utility>
#include <vector>
#include <cassert>
#include <cstring>
#include <cmath>
#include <fcntl.h>
using namespace std;

#define DISK_SIZE 256
int MAX_FILE_SIZE;
int FREE_BLOCKS;// to know how many free blocks we have
int BIGGEST_FD = -1;// keep track of the biggest fd we have
int FDSIZE = 0;// fd array size
int CUR_DSIZE = 0;// keep track of the current disk size;
// ============================================================================
class FsFile {
    int file_size;
    int block_in_use;
    int index_block;
    int block_size;
public:
    FsFile(int _block_size) {
        file_size = 0;
        block_in_use = 0;
        block_size = _block_size;
        index_block = -1;
    }
    int getfile_size() const {
        return file_size;}
    void setFileSize(int fileSize) {
        file_size = fileSize;}
    int getBlockInUse() const {
        return block_in_use;}
    void setBlockInUse(int blockInUse) {
        block_in_use = blockInUse;}
    int getIndexBlock() const {
        return index_block;}
    void setIndexBlock(int indexBlock) {
        index_block = indexBlock;}
    int getBlockSize() const {
        return block_size;}
    void setBlockSize(int blockSize) {
        block_size = blockSize;}
};
// ============================================================================
class FileDescriptor {
    string file_name;
    FsFile *fs_file;
    bool inUse;
public:
    FileDescriptor(string FileName, FsFile *fsi) {
        file_name = std::move(FileName);
        fs_file = fsi;
        inUse = true;
    }
    ~FileDescriptor() {
        delete fs_file;}
    string getFileName() {
        return file_name;}
    bool isInUse() const {
        return inUse;}
    void setInUse(bool inUseu) {
        FileDescriptor::inUse = inUseu;}
    FsFile *getFsFile() const {
        return fs_file;}
};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"
// ============================================================================
class FD_Connector {// a class added by me to keep in the vector it holds the file descriptor and the fd number
public:
    int fd;
    FileDescriptor *file;
    FD_Connector(int fd, FileDescriptor *file) {
        this->fd = fd;
        this->file = file;
    }
    ~FD_Connector() {
        delete file;}
};
class fsDisk {
    FILE *sim_disk_fd;
    bool is_formated;
    int BSize;
    int BitVectorSize;
    int *BitVector;
    int *FD_Vector;
    vector<FD_Connector *> MainDir;
    vector<int> OpenFileDescriptor;
public:
    // ------------------------------------------------------------------------
    fsDisk() {// fsDisk constructor
        sim_disk_fd = fopen(DISK_SIM_FILE, "w+");
        assert(sim_disk_fd);
        for (int i = 0; i < DISK_SIZE; i++) {
            fseek(sim_disk_fd, i, SEEK_SET);
            assert(fwrite("\0", 1, 1, sim_disk_fd)==1);
        }
        fflush(sim_disk_fd);
        is_formated = false;
    }
    ~fsDisk() {
        fclose(sim_disk_fd);
        if (is_formated) {// if the file was not formated we only close thr file that was opened in the constructor.
            OpenFileDescriptor.clear();
            for (auto &i: MainDir) {
                delete i;
            }
            MainDir.clear();
            delete[] BitVector;
            free(FD_Vector);
        }
    }
    // ------------------------------------------------------------------------
    void listAll() {
        int i;
        for (i = 0; i < MainDir.size(); i++) cout << "index: " << i << ": FileName: " << MainDir[i]->file->getFileName()<< " fd: " << MainDir[i]->fd << " , isInUse: " << MainDir[i]->file->isInUse() << endl;
        char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++) {
            fseek(sim_disk_fd, i, SEEK_SET);
            assert(fread(&bufy, 1, 1, sim_disk_fd)==1);
            cout << "(" << bufy << ")";
        }
        cout << "'" << endl;
    }
    // ------------------------------------------------------------------------
    void fsFormat(int blockSize = 4) {// add file descriptor deleter.
        if (is_formated) {// if the file was formated before
            free(FD_Vector);
            delete[] BitVector;
            for (auto &i: MainDir) // delete all files from the main dir.
                delete i;
            MainDir.clear();
            OpenFileDescriptor.clear();
        }
        FD_Vector = nullptr;
        BitVectorSize = DISK_SIZE / blockSize;
        FREE_BLOCKS = BitVectorSize;
        BitVector = new int[BitVectorSize];
        for (int i = 0; i < BitVectorSize; i++) BitVector[i] = 0;
        for (int i = 0; i < DISK_SIZE; i++) {// clear the file
            fseek(sim_disk_fd, i, SEEK_SET);
            assert(fwrite("\0", 1, 1, sim_disk_fd)==1);}
        BIGGEST_FD = -1;   // reset all the needed values.
        FDSIZE = 0;
        is_formated = true;
        CUR_DSIZE = 0;
        BSize = blockSize;
        MAX_FILE_SIZE = blockSize * blockSize;
    }
    // ------------------------------------------------------------------------
    int CreateFile(const string &fileName) {
        if (sim_disk_fd == nullptr || !is_formated)  return -1;// if the disk was not created or formated.
        int i;
        for (i = 0; i < MainDir.size(); i++) {// if the file is already created.
            if (MainDir[i]->file->getFileName() == fileName)
                return -1;}
        FsFile *filef;
        filef = new FsFile(BSize);
        filef->setIndexBlock(-1);// stating value.
        filef->setBlockSize(BSize);// stating value.
        filef->setBlockInUse(0);// stating value.
        filef->setFileSize(0);// stating value.
        FileDescriptor *ins;
        ins = new FileDescriptor(fileName, filef);
        FD_Connector *MD;//  new object of the class the holds the fd and the file descriptor.
        MD = new FD_Connector(-1, ins);
        MainDir.push_back(MD);// Placing the newly created file into the MainDir
        return OpenFile(fileName);
    }
    // ------------------------------------------------------------------------
    int OpenFile(const string &fileName) {
        if (sim_disk_fd == nullptr || !is_formated) return -1;
        int i;
        bool found = false;
        for (i = 0; i < MainDir.size(); i++) {// finding the index of the file in the MainDir.
            if (MainDir[i]->file->getFileName() == fileName) {//if found.
                found = true;
                break;}}
        if (found) {
            if (MainDir[i]->fd != -1) return -1; // already open.
            int fd = FD_finder();// return the index of the empty place.
            if (fd + 1 == FDSIZE) {// if we don't have any place in the FD_vector.
                FD_Vector = (int *) realloc(FD_Vector, sizeof(int) * FDSIZE);// we allocate a new place in the array for the next fd.
                FD_Vector[FDSIZE - 1] = 1;// set the value to one, so we know it was taken
                MainDir[i]->fd = fd;
                if (fd > BIGGEST_FD)
                    BIGGEST_FD = fd;}
            if (i < MainDir.size()) {
                MainDir[i]->file->setInUse(true);//set the value to true.
                MainDir[i]->fd = fd;
                OpenFileDescriptor.push_back(fd);// insert it in the openfile descriptor.
                return fd;}
        }
        return -1;}
    // ------------------------------------------------------------------------
    string CloseFile(int fd) {
        if (sim_disk_fd == nullptr || !is_formated || fd > BIGGEST_FD) return "-1";
        bool found = false;
        int i;
        for (i = 0; i < OpenFileDescriptor.size(); i++) {// looking for the file in the open file descriptor.
            if (OpenFileDescriptor[i] == fd) {// if found
                found = true;
                OpenFileDescriptor.erase(OpenFileDescriptor.begin() + i);// remove it from the open file descriptor.
                break;
            }
        }
        if(found) {
            int j;
            for (j = 0; j < MainDir.size(); j++) {
                if (MainDir[j]->fd == fd) {
                    int File_fd = MainDir[j]->fd;
                    FD_Vector[File_fd] = 0;// set the value in the FD array to zero indicating that this fd is not used.
                    MainDir[j]->fd = -1;
                    MainDir[j]->file->setInUse(false);// set the value to false.
                    break;}
            }
            return MainDir[j]->file->getFileName();}//return the filename
        return "-1";
    }
    // ------------------------------------------------------------------------
    int WriteToFile(int fd, char *buf, int len) {
        if (sim_disk_fd == nullptr || !is_formated || fd > BIGGEST_FD) return -1;
        bool found = false;
        int ori = len;
        for (int i = 0; i < MainDir.size(); i++) {//looking for the file in the main dir.
            if (MainDir[i]->fd == fd) {
                found = true;// if found.
                fd = i;
                break;}}
        if (!found || !MainDir[fd]->file->isInUse())  return -1;// if not found or not in use stop.
        if (MainDir[fd]->file->getFsFile()->getIndexBlock() == -1) {// if it's the first time writing in this file.
            if(FREE_BLOCKS == 0) return -1;
            int index = BV_finder();// search for a free block index.
            CUR_DSIZE += BSize;//adding the used block to the current disk size.
            MainDir[fd]->file->getFsFile()->setIndexBlock(index * BSize);//set value.
            MainDir[fd]->file->getFsFile()->setBlockInUse(1);//add one to the number of blocks used by the file.
        }
        int F_SIZE = MainDir[fd]->file->getFsFile()->getfile_size();
        if (F_SIZE + len > MAX_FILE_SIZE) len = MAX_FILE_SIZE - F_SIZE; // if adding the new word to the file makes its size bigger than the maximum file size // only add what we can.
        if (CUR_DSIZE + len > DISK_SIZE)  len = DISK_SIZE - CUR_DSIZE;//checking if there's space in the disk itself // if not add what we can.
        unsigned char c_files;
        int amount_written = 0;
        int ind;
        int Bsize = MainDir[fd]->file->getFsFile()->getBlockSize();
        int ptr_first = MainDir[fd]->file->getFsFile()->getIndexBlock();// pointer to the first index in the index block.
        int i = 0;
        while (amount_written < len) {
            for (; i < Bsize;) {// get the block to write in.
                fseek(sim_disk_fd, i + ptr_first, SEEK_SET);
                assert(fread(&c_files, 1, 1, sim_disk_fd)==1);
                if (c_files == '\0') {// if nothing is written in the block
                    if (FREE_BLOCKS == 0) return  -1;
                    ind = BV_finder();// look for an empty block on the disk.
                    ind += 32;
                    fseek(sim_disk_fd, i + ptr_first, SEEK_SET);
                    assert(fwrite((unsigned char *) &ind, 1, 1, sim_disk_fd)==1);// write its value on the file to point to the index.
                    int n_BIU = MainDir[fd]->file->getFsFile()->getBlockInUse();
                    MainDir[fd]->file->getFsFile()->setBlockInUse(n_BIU + 1);// adding one to the amount of blocks used by the file.
                    i++;
                    break;}
                if (c_files != '\0') {// if the address is pointing to a block.
                    ind = (int) (c_files);// take the blocks value.
                    i++;
                    break;}
            }
            ind = (ind - 32) * Bsize;
            char B_chr;
            for (int j = 0; j < Bsize && amount_written < len; j++) {//search through the block for empty places.
                fseek(sim_disk_fd, j + ind, SEEK_SET);
                assert(fread(&B_chr, 1, 1, sim_disk_fd)==1);
                if (B_chr == '\0') {//if nothing is written on this index
                    fseek(sim_disk_fd, j + ind, SEEK_SET);
                    assert(fwrite(&buf[amount_written++], 1, 1, sim_disk_fd)==1);
                    CUR_DSIZE++;
                    int N_fs = MainDir[fd]->file->getFsFile()->getfile_size();
                    MainDir[fd]->file->getFsFile()->setFileSize(N_fs + 1);// adding one to the file size.
                }
            }
        }
        if (len == ori)return len;
        else
        return -1;
    }
    // ------------------------------------------------------------------------
    int DelFile(const string &FileName) {
        if (sim_disk_fd == nullptr || !is_formated) return -1;
        bool found = false;
        int i;
        for (i = 0; i < MainDir.size(); i++) {// checking if the file exists.
            if (MainDir[i]->file->getFileName() == FileName) {
                found = true;//if found.
                break;}}
        if(!found||MainDir[i]->file->isInUse()) return -1;
            int BlocksInUse = MainDir[i]->file->getFsFile()->getBlockInUse();
            if(BlocksInUse > 0) {
                BitVector[MainDir[i]->file->getFsFile()->getIndexBlock() / BSize] = 0;// setting the value of the bitvector to zero indicating that the block is free.
                FREE_BLOCKS += BlocksInUse;// adding the amount of blocks that were used by the file to the free blocks' var.
            }
            if (MainDir[i]->file->getFsFile()->getfile_size() != 0)// checking if the file has something written on it.
                CUR_DSIZE -= BSize;
            unsigned char c_files;
            int Amount_to_Remove = MainDir[i]->file->getFsFile()->getfile_size();// the amount of chars we want to remove.
            int ptr_first = MainDir[i]->file->getFsFile()->getIndexBlock();// first index of the index block.
            int k = 0;
            while (BlocksInUse - 1 > k) {// run over the whole file content and remove it.
                fseek(sim_disk_fd, k + ptr_first, SEEK_SET);
                assert(fread(&c_files, 1, 1, sim_disk_fd)==1);
                fseek(sim_disk_fd, k + ptr_first, SEEK_SET);
                assert(fwrite("\0", 1, 1, sim_disk_fd)==1);
                int ind = (int) c_files;
                ind -= 32;
                cout << "vector size = " << BitVectorSize << endl;
                BitVector[ind] = 0;
                ind = ind * BSize;
                for (int p = 0; p < BSize && Amount_to_Remove > 0; p++) {
                    fseek(sim_disk_fd, p + ind, SEEK_SET);
                    assert(fwrite("\0", 1, 1, sim_disk_fd)==1);
                    CUR_DSIZE--;// every char we remove we add one to the disk size.
                    Amount_to_Remove--;
                }
                k++;
            }
            delete MainDir[i];// deleting it from the main dir vector.
            MainDir.erase(MainDir.begin() + i);// erasing it
            return 1;
    }
    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len) {
        buf[0] = '\0';
        if (sim_disk_fd == nullptr || !is_formated || fd > BIGGEST_FD)
            return -1;
        bool found = false;
        int j;
        for (j = 0; j < MainDir.size(); j++) {// looking for the file we want to read from.
            if (MainDir[j]->fd == fd) {
                found = true;
                break;}
        }
        if (!found || !MainDir[j]->file->isInUse()) return -1;// if it's not found or the file is closed we can't read from it.
        int cur_FSize = MainDir[j]->file->getFsFile()->getfile_size();
        if (len > cur_FSize) len = cur_FSize; // if the amount we are trying to read is bigger than the file size.// read what we can.
        buf[len] = '\0';
        int BLOCKS_TO_READ = len / BSize;// the amount of blocks to read.
        if (len % BSize != 0) BLOCKS_TO_READ++;
        int ptr_first = MainDir[j]->file->getFsFile()->getIndexBlock();
        int i = 0;
        unsigned char c_files;
        int ori = len;
        while (i < BLOCKS_TO_READ) {// running over the whole file to read the desired amount.
            fseek(sim_disk_fd, i + ptr_first, SEEK_SET);
            assert(fread(&c_files, 1, 1, sim_disk_fd) == 1);
            int ind = (int) c_files;
            ind -= 32;
            ind = ind * BSize;
            for (int l = 0; l < BSize && len > 0; l++) {
                fseek(sim_disk_fd, l + ind, SEEK_SET);
                assert(fread(&buf[(i * BSize) + l], 1, 1, sim_disk_fd) == 1);
                len--;
            }
            i++;
        }
        return ori;
    }
private:
    int FD_finder() {//a function to find a place in the fd array to keep track of the fds.
        int i;
        bool found = false;
        for (i = 0; i < FDSIZE; i++) {
            if (FD_Vector[i] == 0) {
                found = true;
                FD_Vector[i] = 1;
                break;}
        }
        if (found)// if a free place was found
            return i;// return its index.
        else
            return FDSIZE++;// else return the next in line.
    }
    int BV_finder() {// a function to search for empty
        int i;
        for (i = 0; i < BitVectorSize; i++) {// first empty space.
            if (BitVector[i] == 0) {
                BitVector[i] = 1;
                FREE_BLOCKS--;
                break;}
        }
        return i;// if found return its index.
    }
};

int main() {
    int blockSize;
//    int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;

    fsDisk *fs;
    fs = new fsDisk();
    int cmd_;
    while (true) {
        cin >> cmd_;
        switch (cmd_) {
            case 0:   // exit
                delete fs;
                exit(0);

            case 1:  // list-file
                fs->listAll();
                break;

            case 2:    // format
                cin >> blockSize;
                fs->fsFormat(blockSize);
                break;

            case 3:    // creat-file
                cin >> fileName;
                _fd = fs->CreateFile(fileName);
                cout << "CreateFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 4:  // open-file
                cin >> fileName;
                _fd = fs->OpenFile(fileName);
                cout << "OpenFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 5:  // close-file
                cin >> _fd;
                fileName = fs->CloseFile(_fd);
                cout << "CloseFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;

            case 6:   // write-file
                cin >> _fd;
                cin >> str_to_write;
                fs->WriteToFile(_fd, str_to_write, strlen(str_to_write));
                break;

            case 7:    // read-file
                cin >> _fd;
                cin >> size_to_read;
                fs->ReadFromFile(_fd, str_to_read, size_to_read);
                cout << "ReadFromFile: " << str_to_read << endl;
                break;

            case 8:   // delete file
                cin >> fileName;
                _fd = fs->DelFile(fileName);
                cout << "DeletedFile: " << fileName << " with File Descriptor #: " << _fd << endl;
                break;
            default:
                break;
        }
    }
}