#include <iostream>
#include <vector>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>

using namespace std;

#define DISK_SIZE 16
int MAX_FILE_SIZE;
int FREE_BLOCKS;
int BIGGEST_FD = -1;


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
        return file_size;
    }

    void setFileSize(int fileSize) {
        file_size = fileSize;
    }

    int getBlockInUse() const {
        return block_in_use;
    }

    void setBlockInUse(int blockInUse) {
        block_in_use = blockInUse;
    }

    int getIndexBlock() const {
        return index_block;
    }

    void setIndexBlock(int indexBlock) {
        index_block = indexBlock;
    }

    int getBlockSize() const {
        return block_size;
    }

    void setBlockSize(int blockSize) {
        block_size = blockSize;
    }

};

// ============================================================================

class FileDescriptor {
    string file_name;
    FsFile *fs_file;
    bool inUse;

public:

    FileDescriptor(string FileName, FsFile *fsi) {
        file_name = FileName;
        fs_file = fsi;
        inUse = true;
    }

    ~FileDescriptor() {
        delete fs_file;
    }

    string getFileName() {
        return file_name;
    }

    bool isInUse() const {
        return inUse;
    }

    void setInUse(bool inUse) {
        FileDescriptor::inUse = inUse;
    }

    FsFile *getFsFile() const {
        return fs_file;
    }

};

#define DISK_SIM_FILE "DISK_SIM_FILE.txt"

// ============================================================================

class FD_Connector {
public:
    int fd;
    FileDescriptor *file;

    FD_Connector(int fd, FileDescriptor *file) {
        this->fd = fd;
        this->file = file;
    }

    ~FD_Connector() {
        delete file;
    }
};

class fsDisk {
    FILE *sim_disk_fd;
    bool is_formated;
    int BSize;

    // BitVector - "bit" (int) vector, indicate which block in the disk is free
    //              or not.  (i.e. if BitVector[0] == 1 , means that the
    //             first block is occupied.
    int BitVectorSize;
    int *BitVector;
    int *FD_Vector;

    // (5) MainDir --
    // Structure that links the file name to its FsFile
    vector<FD_Connector *> MainDir;

    // (6) OpenFileDescriptors --
    //  when you open a file,
    // the operating system creates an entry to represent that file
    // This entry number is the file descriptor.

    vector<int> OpenFileDescriptor;


public:
    // ------------------------------------------------------------------------
    fsDisk() {
        /*
         * initialize MainDir and OpenFileDir.
         */
        sim_disk_fd = fopen(DISK_SIM_FILE, "w+");
        assert(sim_disk_fd);
        for (int i = 0; i < DISK_SIZE; i++) {
            fseek(sim_disk_fd, i, SEEK_SET);
            int ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        fflush(sim_disk_fd);
        is_formated = false;
    }

    ~fsDisk() {
        fclose(sim_disk_fd);
        OpenFileDescriptor.clear();
        for (auto &i: MainDir) {
            delete i;
        }
        MainDir.clear();
        delete[] BitVector;
        delete[] FD_Vector;
    }

    // ------------------------------------------------------------------------
    void listAll() {
        for (auto &i: MainDir) {
            cout << "file size " << i->file->getFileName() << " fd = " << i->fd << " size ==" << i->file->getFsFile()->getfile_size() << endl;
        }
        cout << "FREEBLOCKS ---- " << FREE_BLOCKS << endl;
        int counter = 0;
        for (int l = 0; l < BitVectorSize; l++) {
            if (BitVector[l] == 0) {
                counter++;
            }
        }
        cout << "actual free blocks --" << counter << endl;
        int i = 0;
        int use;
        for (i = 0; i < MainDir.size(); i++) {
            use = 0;
            if (MainDir[i]->file->isInUse()) {
                use = 1;
            }
            cout << "index: " << i << ": FileName: " << MainDir[i]->file->getFileName() << " , isInUse: " << use << endl;
        }
        char bufy;
        cout << "Disk content: '";
        for (i = 0; i < DISK_SIZE; i++) {
            cout << "(";
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fread(&bufy, 1, 1, sim_disk_fd);
            cout << bufy;
            cout << ")";
        }
        cout << "'" << endl;
    }

    // ------------------------------------------------------------------------
    void fsFormat(int blockSize = 4) {// add file descriptor deleter.

        if (is_formated) {
            for (auto i: MainDir) {
                DelFile(i->file->getFileName());
            }
            listAll();
        }
        if (!is_formated) {
            BitVectorSize = DISK_SIZE / blockSize;
            FREE_BLOCKS = BitVectorSize;
            BitVector = new int[BitVectorSize];
            FD_Vector = new int[BitVectorSize];
        }
        for (int i = 0; i < BitVectorSize; i++) {
            BitVector[i] = 0;
            FD_Vector[i] = 0;
        }
        for (int i = 0; i < DISK_SIZE; i++) {
            int ret_val = fseek(sim_disk_fd, i, SEEK_SET);
            ret_val = fwrite("\0", 1, 1, sim_disk_fd);
            assert(ret_val == 1);
        }
        is_formated = true;
//        FREE_BLOCKS = DISK_SIZE/blockSize;
        BSize = blockSize;
        MAX_FILE_SIZE = blockSize * blockSize;
    }

    // ------------------------------------------------------------------------
    int CreateFile(string fileName) {// fix file fd !!!
        if (sim_disk_fd == NULL || !is_formated) {
            return -1;
        }
        int i;
        for (i = 0; i < MainDir.size(); i++) {// if the file is already created.
            if (MainDir[i]->file->getFileName() == fileName)
                return -1;
        }
        i = BV_finder();
        int fd = FD_finder();// return the index of the empty place.
        int index = i * BSize;
        FsFile *filef = new FsFile(BSize);
        filef->setIndexBlock(index);
        filef->setBlockSize(BSize);
        filef->setBlockInUse(1);
        filef->setFileSize(0);
        FileDescriptor *ins = new FileDescriptor({fileName, filef});
        FD_Connector *MD = new FD_Connector(fd, ins);
        MainDir.push_back(MD);// Placing the newly created file into the MainDir
        OpenFileDescriptor.push_back(fd);// Placing the newly created file into the OpenFileDescriptor

        if (fd > BIGGEST_FD)
            BIGGEST_FD = fd;

        return fd;
    }

    // ------------------------------------------------------------------------
    int OpenFile(string fileName) {
        if (sim_disk_fd == NULL || !is_formated)
            return -1;
        int i;
        bool found = false;
        for (i = 0; i < MainDir.size(); i++) {// finding the index of the file in the MainDir.
            if (MainDir[i]->file->getFileName() == fileName) {
                found = true;
                break;
            }
        }
        if (found) {
            if (MainDir[i]->file->isInUse()) {// already open.
                return -1;
            }
            int fd = MainDir[i]->fd;
            if (i < MainDir.size()) {
                MainDir[i]->file->setInUse(true);
                OpenFileDescriptor.push_back(fd);
                return fd;
            }
        }
        return -1;
    }
    // ------------------------------------------------------------------------
    string CloseFile(int fd) {// might need some fixing.
        if (sim_disk_fd == NULL || !is_formated || fd > BIGGEST_FD) {
            return "-1";
        }
        int i;
        for (i = 0; i < OpenFileDescriptor.size(); i++) {
            if (OpenFileDescriptor[i] == fd) {
                OpenFileDescriptor.erase(OpenFileDescriptor.begin() + i);
                break;
            }
        }
        int j;
        for (j = 0; j < MainDir.size(); j++) {
            if (MainDir[j]->fd == fd) {
                MainDir[j]->file->setInUse(false);
                break;
            }
        }
        return MainDir[j]->file->getFileName();
    }

    // ------------------------------------------------------------------------
    int WriteToFile(int fd, char *buf, int len) {
        if (sim_disk_fd == NULL || !is_formated || fd > BIGGEST_FD) {
            return -1;
        }
        for (int i = 0; i < MainDir.size(); i++) {
            if (MainDir[i]->fd == fd) {
                fd = i;
                break;
            }
        }
        if(!MainDir[fd]->file->isInUse())
            return -1;
        int F_SIZE = MainDir[fd]->file->getFsFile()->getfile_size();
        if (F_SIZE + len > MAX_FILE_SIZE) {
            len = MAX_FILE_SIZE - F_SIZE-(FREE_BLOCKS*BSize);
        }

        if(FREE_BLOCKS == 0){
            len = F_SIZE%BSize;
        }

        char c_files;
        char toBi;
        int amount_written = 0;
        int ind;
        int Bsize = MainDir[fd]->file->getFsFile()->getBlockSize();
        int ptr_first = MainDir[fd]->file->getFsFile()->getIndexBlock();
        int i = 0;
        while (amount_written < len) {
            for (; i < Bsize;) {// get the block to write in.
                fseek(sim_disk_fd, i + ptr_first, SEEK_SET);
                fread(&c_files, 1, 1, sim_disk_fd);
                if (c_files == '\0') {// if nothing is written in the block
                    ind = BV_finder();
                    fseek(sim_disk_fd, i + ptr_first, SEEK_SET);
                    fwrite((unsigned char *) &ind, 1, 1, sim_disk_fd);
                    int n_BIU = MainDir[fd]->file->getFsFile()->getBlockInUse();
                    MainDir[fd]->file->getFsFile()->setBlockInUse(n_BIU + 1);
                    i++;
                    break;
                }
                if (c_files != '\0') {
                    ind = (int) (c_files);
                    i++;
                    break;
                }
            }
            ind = ind * Bsize;
            //look for empty places in the block to write.
            char B_chr;
            for (int j = 0; j < Bsize && amount_written < len; j++) {//search through the block for empty places.
                fseek(sim_disk_fd, j + ind, SEEK_SET);
                fread(&B_chr, 1, 1, sim_disk_fd);
                if (B_chr == '\0') {
                    fseek(sim_disk_fd, j + ind, SEEK_SET);
                    fwrite(&buf[amount_written++], 1, 1, sim_disk_fd);
                    int N_fs = MainDir[fd]->file->getFsFile()->getfile_size();
                    MainDir[fd]->file->getFsFile()->setFileSize(N_fs + 1);
                }
            }
        }
        return 1;
    }

    // ------------------------------------------------------------------------
    int DelFile(string FileName) {// fix doing problems with the MainDir.
        if (sim_disk_fd == NULL || !is_formated) {
            return -1;
        }
        int i;
        int File_fd;
        for (i = 0; i < MainDir.size(); i++) {
            if (MainDir[i]->file->getFileName() == FileName) {
                File_fd = MainDir[i]->fd;
                break;
            }
        }
        int BlocksInUse = MainDir[i]->file->getFsFile()->getBlockInUse();
        FD_Vector[File_fd] = 0;
        BitVector[File_fd] = 0;
        FREE_BLOCKS += BlocksInUse;
        unsigned char c_files;
        for (int j = 0; j < OpenFileDescriptor.size(); j++) {
            if (OpenFileDescriptor[j] == File_fd) {
                OpenFileDescriptor.erase(OpenFileDescriptor.begin() + j);
                break;
            }
        }
        int Amount_to_Remove = MainDir[i]->file->getFsFile()->getfile_size();
        int ptr_first = MainDir[i]->file->getFsFile()->getIndexBlock();
        int k = 0;
        while (BlocksInUse - 1 > k) {
            fseek(sim_disk_fd, k + ptr_first, SEEK_SET);
            fread(&c_files, 1, 1, sim_disk_fd);
            fseek(sim_disk_fd, k + ptr_first, SEEK_SET);
            fwrite("\0", 1, 1, sim_disk_fd);
            int ind = (int) c_files;
            BitVector[ind] = 0;
            ind = ind * BSize;
            for (int p = 0; p < BSize && Amount_to_Remove > 0; p++) {
                fseek(sim_disk_fd, p + ind, SEEK_SET);
                fwrite("\0", 1, 1, sim_disk_fd);
                Amount_to_Remove--;
            }
            k++;
        }
//         cout << "file size before"  << MainDir[i]->file->getFsFile()->getfile_size() << endl;
        MainDir.erase(MainDir.begin() + i);
//        MainDir[i]->file->getFsFile()->setFileSize(0);
//        cout << "file size before"  << MainDir[i]->file->getFsFile()->getfile_size() << endl;
        return File_fd;
    }

    // ------------------------------------------------------------------------
    int ReadFromFile(int fd, char *buf, int len) {
        if (sim_disk_fd == NULL || !is_formated || fd > BIGGEST_FD) {// fix size after delete fun.
            return -1;
        }
        int cur_FSize = MainDir[fd]->file->getFsFile()->getfile_size();
        if (len > cur_FSize) {
            len = cur_FSize;
        }
        buf[len] = '\0';
        int BLOCKS_TO_READ = len / BSize;
        if (len % BSize != 0) {
            BLOCKS_TO_READ++;
        }
        int ptr_first = MainDir[fd]->file->getFsFile()->getIndexBlock();
        int i = 0;
        unsigned char c_files;
        while (i < BLOCKS_TO_READ) {
            fseek(sim_disk_fd, i + ptr_first, SEEK_SET);
            fread(&c_files, 1, 1, sim_disk_fd);
            int ind = (int) c_files;
            ind = ind * BSize;
            for (int j = 0; j < BSize && len > 0; j++) {
                fseek(sim_disk_fd, j + ind, SEEK_SET);
                fread(&buf[(i * BSize) + j], 1, 1, sim_disk_fd);
                len--;
            }
            i++;
        }
        return 1;
    }

private:
    void Clean_BV(int index, int BinUse) {


    }

    int FD_finder() {
        int i;
        for (i = 0; i < BitVectorSize; i++) {
            if (FD_Vector[i] == 0) {
                FD_Vector[i] = 1;
                break;
            }
        }
        return i;
    }

    int BV_finder() {
        int i;
        for (i = 0; i < BitVectorSize; i++) {// first empty space.
            if (BitVector[i] == 0) {
                BitVector[i] = 1;
                FREE_BLOCKS--;
                break;
            }
        }

        return i;
    }
};

int main() {
    int blockSize;
    int direct_entries;
    string fileName;
    char str_to_write[DISK_SIZE];
    char str_to_read[DISK_SIZE];
    int size_to_read;
    int _fd;

    fsDisk *fs = new fsDisk();
    int cmd_;
    while (1) {
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