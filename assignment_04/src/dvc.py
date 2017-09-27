duplicates = open('duplicates.py.txt', 'w')

lcount, dcount = 0, 0
subject_counts = {}
unique_codes   = set()

with open('../data/dvc-schedule.txt', 'r') as f:
    for line in f:
        fields = line.strip().split('\t')
        unique_code = fields[0] + fields[1]
        subject     = fields[2].split('-')[0]

        if unique_code in unique_codes:
            duplicates.write("DUPLICATE: '%s'\n"%line.strip())
        else:
            unique_codes.add(unique_code)
            if subject in subject_counts:
                subject_counts[subject] += 1
            else:
                subject_counts[subject] = 1
    # for line in set([ line.strip() for line in f ]):
    #     subject = line.split('\t')[2].split('-')[0]
    #     if subject in subject_counts:
    #         subject_counts[subject] += 1
    #     else:
    #         subject_counts[subject] = 1

duplicates.close()

print('\n'.join([ '%s: %s'%(k, v) for k, v in 
    sorted(subject_counts.iteritems(), key = lambda (k,v): k)
]))
print("%s lines, %s duplicates"%(lcount, dcount))

