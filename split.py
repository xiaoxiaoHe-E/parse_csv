import csv
import io
import time
import get_eol
import subprocess

file_name = 'test.csv'
delimiter = ','
quote = "'"
escape = '\\'
eol = '\n'
line_per_file = 5000
buffer_len = 16384  # 16k bytes
read_buffer = 64*1024  # larger than a protobuf chunk size


def split_file_fork_c():
    print("split in fork c sub process")
    st = time.time()
    p = subprocess.Popen("./a.out -f %s" % file_name, stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
    cur_chunk_st = 0
    line = p.stdout.readline().strip('\n')
    cur_chunk_ed = int(line)
    chunk_cnt = 0
    with open(file_name, 'r') as in_f:
        csv_data = in_f.read(read_buffer)
        read_chunk_st = 0
        read_chunk_ed = len(csv_data)
        c = ""

        while 1:
            if cur_chunk_ed < read_chunk_ed:
                # this read chunk is enough for protobuf chunk
                # cur_chunk_st has problem
                c += csv_data[cur_chunk_st-read_chunk_st:cur_chunk_ed-read_chunk_st]
                chunk_cnt += 1
                cur_chunk_st += len(c)
                # do sth
                c = ""
                line = p.stdout.readline().strip('\n')
                if line == '':
                    break
                cur_chunk_ed = int(line)
            elif cur_chunk_ed == read_chunk_ed:
                c += csv_data[cur_chunk_st-read_chunk_st:]
                chunk_cnt += 1
                cur_chunk_st += len(c)
                c = ""
                line = p.stdout.readline().strip('\n')
                if line == '':
                    break
                cur_chunk_ed = int(line)
                csv_data = in_f.read(read_buffer)
                read_chunk_st = read_chunk_ed
                read_chunk_ed += len(csv_data)
            else:
                # this read chunk is not enough
                c += csv_data
                csv_data = in_f.read(read_buffer)
                read_chunk_st = read_chunk_ed
                read_chunk_ed += len(csv_data)

    ed = time.time()
    p.kill()
    cost_time = ed - st
    print("processed chunk: %d" % chunk_cnt)
    print('total cost time: {}s\n'.format(float('%.2f' % cost_time)))


def split_file_c_function():
    st = time.time()
    line_count = 0
    buffer_st = 0
    buffer_ed = buffer_len
    with open(file_name, 'r') as in_f:
        csv_data = in_f.read()
        data = csv_data[buffer_st:buffer_ed]
        while data:
            eol_list = get_eol.get_eol(buffer_len, data, quote, escape, eol)
            eol_count = eol_list[0]
            line_count += eol_count-1
            # eol_list[0] recods the length of the list includs the eol_list[0]
            # eol_list is the relative index inside the csv_data
            last_eol = eol_list[eol_count-1]
            buffer_st = buffer_st + last_eol + 1
            buffer_ed = buffer_st + buffer_len
            data = csv_data[buffer_st:buffer_ed]

    # may have some data after the last eol
    et = time.time()
    print('split file in c function:')
    print('Processed %d lines.' % line_count)
    cost_time = et - st
    print('total cost time: {}s'.format(float('%.2f' % cost_time)))
    print('')


def split_file_csv():
    data2write = []
    st = time.time()
    csv.register_dialect('mycsv', delimiter=delimiter, quotechar=quote, lineterminator=eol)
    with open(file_name,'r') as csv_file:
        csv_reader = csv.reader(csv_file,  dialect='mycsv')
        line_count = 0
        file_count = 0
        for line in csv_reader:
            line_count += 1
            if line_count>0 and line_count%line_per_file == 0:
                # with open('test/out%d.csv'%file_count,'w') as outfile:
                #     writer = csv.writer(outfile)
                #     writer.writerows(data2write)
                file_count += 1
                data2write = []
            data2write.append(line)

        if len(data2write)>0:
            # with open('out%d.csv'%file_count,'w') as outfile:
            #     writer = csv.writer(outfile)
            #     writer.writerows(data2write)
            file_count += 1
    et = time.time()
    print('split file in csv:')    
    print('Processed %d lines.' % line_count)
    cost_time = et - st
    print('cost time: {}s'.format(float('%.2f' % cost_time)))
    print('')


def splie_file_readlines():
    st = time.time()
    with io.open(file_name,'r', newline='\n') as in_f:
        csv_file = in_f.readlines()
        rdt = time.time()
        file_count = 0
        line_count = len(csv_file)
        lct = time.time()
        for i in range(0, line_count, line_per_file):
            # with open('out'+ str(file_count) + '.csv', 'w+') as f:
            #     f.writelines(csv_file[i:i+line_per_file])
            file_count += 1
    et = time.time()

    print('split file in open readlines:')
    print('Processed %d lines.' % line_count)
    read_time = float(rdt) - float(st)
    print('read time: {}s'.format(float('%.2f' % read_time)))
    len_time = float(lct) - float(rdt)
    print('calcute len time: {}s'.format(float('%.2f' % len_time)))
    cost_time = et - st
    print('cost time: {}s'.format(float('%.2f' % cost_time)))
    print('')

if __name__ == "__main__":
    split_file_csv()
    splie_file_readlines()
    split_file_c_function()

    split_file_fork_c()