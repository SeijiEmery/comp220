
subject_counts = {}
with open('../data/dvc-schedule.txt', 'r') as f:
    for line in set([ line.strip() for line in f ]):
        subject = line.split('\t')[2].split('-')[0]
        if subject in subject_counts:
            subject_counts[subject] += 1
        else:
            subject_counts[subject] = 1

print('\n'.join([ '%s: %s'%(k, v) for k, v in 
    sorted(subject_counts.iteritems(), key = lambda (k,v): k)
]))
