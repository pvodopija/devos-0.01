#include <errno.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/kernel.h>
#include <linux/security.h>
#include <asm/segment.h>
#include <sys/times.h>
#include <sys/utsname.h>
#include <sys/stat.h>

/* devos sys_calls */
int sys_encr(const char *file_path){
	struct m_inode* root_node = iget(0x301, 1);	
	current->root = root_node;

	struct m_inode* dir_node = namei(file_path);
	int status;

	key_to_use = (!current->key[0])?encryption_key:current->key;
	hash_to_use = (!current->key[0])?h_key:current->h_key;

	if(!dir_node){
		printk("error: File not found.\n");
		status = -1;
	}else if(!strlen(key_to_use)){
		printk("error: Key not set.\n");
		status = -1;
	}else{
		if(S_ISDIR(dir_node->i_mode))
			encrypt_directory(dir_node);
		else{
			struct e_dir new_dir;
			new_dir.i_num = dir_node->i_num;
			string_copy(new_dir.h_key,hash_to_use,HKEY_SIZE);
			encrypt_file(dir_node, &new_dir);
		}
				
	}
	
	iput(dir_node);
	iput(root_node);
	current->pwd = NULL;
	current->root = NULL;
	
	return status;
	
}

int sys_decr(const char* file_path){
	struct m_inode* root_node = iget(0x301, 1);	
	current->root =root_node;

	struct m_inode* dir_node = namei(file_path);

	int status;
	key_to_use = (!current->key[0])?encryption_key:current->key;
	hash_to_use = (!current->key[0])?h_key:current->h_key;

	if(!dir_node){
		printk("error: File not found.\n");
		status = -1;
	}else if(!strlen(key_to_use)){
		printk("error: Key not set.\n");
		status = -1;
	}else{
		if(S_ISDIR(dir_node->i_mode))
			decrypt_directory(dir_node);
		else
			decrypt_file(dir_node);

	}

	iput(dir_node);
	iput(root_node);
	current->pwd = NULL;
	current->root = NULL;


	return status;
}

int sys_keyset(const char* key, int glob_flag){
	char _key[KEY_SIZE];

	userspace_string_cpy(_key, key);
	
	return set_key(_key, glob_flag);
}

int sys_keyclear(int glob_flag){
	clear_key(glob_flag);
	
	/* struct m_inode* dir_node = iget(0x301, ENC_TABLE_INODE);
	if(!dir_node){
		printk("NOT FOUND\n");
		return 0;
	}

	struct buffer_head* buff_head = bread(dir_node->i_dev, dir_node->i_zone[0]);

	clear_block(buff_head->b_data);

	brelse(buff_head);
	iput(dir_node);
	


	struct m_inode* dir_node = iget(0x301, 1);
	if(!dir_node){
		printk("NOT FOUND\n");
		return 0;
	}

	struct buffer_head* buff_head = bread(dir_node->i_dev, dir_node->i_zone[0]);
	struct dir_entry* curr_dir = (struct dir_entry*) buff_head->b_data;

	while((curr_dir++)->inode != ENC_TABLE_INODE);
	--curr_dir;

	printk("%d\n", curr_dir->inode);

	curr_dir->inode = 0;
	curr_dir->name[0] = '\0';
	curr_dir->name[1] = '\0';
	curr_dir->name[2] = '\0';
	curr_dir->name[3] = '\0';
	curr_dir->name[4] = '\0';
	curr_dir->name[5] = '\0';
	curr_dir->name[6] = '\0';
	curr_dir->name[7] = '\0';
	curr_dir->name[8] = '\0';
	curr_dir->name[9] = '\0';
	curr_dir->name[10] = '\0';
	curr_dir->name[11] = '\0';
	curr_dir->name[12] = '\0';
	curr_dir->name[13] = '\0';

	brelse(buff_head);
	iput(dir_node);
	*/

	return 0;
}

int sys_keygen(int level){

	char key[KEY_SIZE];
	if(rnd_key_gen(key, level)){
		printk("Generated key: %s\n", key);
		set_key(key, K_GLOBAL);
		return 1;
	}

	return -1;
}

int sys_load_table(){
	load_hash_table();
	return 0;
}

/* end */

int sys_ftime()
{
	return -ENOSYS;
}

int sys_mknod()
{
	return -ENOSYS;
}

int sys_break()
{
	return -ENOSYS;
}

int sys_mount()
{
	return -ENOSYS;
}

int sys_umount()
{
	return -ENOSYS;
}

int sys_ustat(int dev,struct ustat * ubuf)
{
	return -1;
}

int sys_ptrace()
{
	return -ENOSYS;
}

int sys_stty(int tty_index, long settings){
	tty_table[tty_index].settings = settings;	/* change tty struct settings */

	return 0;
}

int sys_gtty()
{
	return -ENOSYS;
}

int sys_rename()
{
	return -ENOSYS;
}

int sys_prof()
{
	return -ENOSYS;
}

int sys_setgid(int gid)
{
	if (current->euid && current->uid)
		if (current->gid==gid || current->sgid==gid)
			current->egid=gid;
		else
			return -EPERM;
	else
		current->gid=current->egid=gid;
	return 0;
}

int sys_acct()
{
	return -ENOSYS;
}

int sys_phys()
{
	return -ENOSYS;
}

int sys_lock()
{
	return -ENOSYS;
}

int sys_mpx()
{
	return -ENOSYS;
}

int sys_ulimit()
{
	return -ENOSYS;
}

int sys_time(long * tloc)
{
	int i;

	i = CURRENT_TIME;
	if (tloc) {
		verify_area(tloc,4);
		put_fs_long(i,(unsigned long *)tloc);
	}
	return i;
}

int sys_setuid(int uid)
{
	if (current->euid && current->uid)
		if (uid==current->uid || current->suid==current->uid)
			current->euid=uid;
		else
			return -EPERM;
	else
		current->euid=current->uid=uid;
	return 0;
}

int sys_stime(long * tptr)
{
	if (current->euid && current->uid)
		return -1;
	startup_time = get_fs_long((unsigned long *)tptr) - jiffies/HZ;
	return 0;
}

int sys_times(struct tms * tbuf)
{
	if (!tbuf)
		return jiffies;
	verify_area(tbuf,sizeof *tbuf);
	put_fs_long(current->utime,(unsigned long *)&tbuf->tms_utime);
	put_fs_long(current->stime,(unsigned long *)&tbuf->tms_stime);
	put_fs_long(current->cutime,(unsigned long *)&tbuf->tms_cutime);
	put_fs_long(current->cstime,(unsigned long *)&tbuf->tms_cstime);
	return jiffies;
}

int sys_brk(unsigned long end_data_seg)
{
	if (end_data_seg >= current->end_code &&
	    end_data_seg < current->start_stack - 16384)
		current->brk = end_data_seg;
	return current->brk;
}

/*
 * This needs some heave checking ...
 * I just haven't get the stomach for it. I also don't fully
 * understand sessions/pgrp etc. Let somebody who does explain it.
 */
int sys_setpgid(int pid, int pgid)
{
	int i;

	if (!pid)
		pid = current->pid;
	if (!pgid)
		pgid = pid;
	for (i=0 ; i<NR_TASKS ; i++)
		if (task[i] && task[i]->pid==pid) {
			if (task[i]->leader)
				return -EPERM;
			if (task[i]->session != current->session)
				return -EPERM;
			task[i]->pgrp = pgid;
			return 0;
		}
	return -ESRCH;
}

int sys_getpgrp(void)
{
	return current->pgrp;
}

int sys_setsid(void)
{
	if (current->uid && current->euid)
		return -EPERM;
	if (current->leader)
		return -EPERM;
	current->leader = 1;
	current->session = current->pgrp = current->pid;
	current->tty = -1;
	return current->pgrp;
}

int sys_oldolduname(void* v)
{
	printk("calling obsolete system call oldolduname\n");
	return -ENOSYS;
//	return (0);
}

int sys_uname(struct utsname * name)
{
	static struct utsname thisname = {
		"linux 0.01-3.x","nodename","release ","3.x","i386"
	};
	int i;

	if (!name) return -1;
	verify_area(name,sizeof *name);
	for(i=0;i<sizeof *name;i++)
		put_fs_byte(((char *) &thisname)[i],i+(char *) name);
	return (0);
}

int sys_umask(int mask)
{
	int old = current->umask;

	current->umask = mask & 0777;
	return (old);
}

int sys_null(int nr)
{
	static int prev_nr=-2;
	if (nr==174 || nr==175) return -ENOSYS;

	if (prev_nr!=nr) 
	{
		prev_nr=nr;
//		printk("system call num %d not available\n",nr);
	}
	return -ENOSYS;
}

