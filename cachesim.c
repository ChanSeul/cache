#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int cache_size,block_size,set_size,
    index_size;
char trace_file[20];

typedef struct memory_data{
	int index;
	int data;
	struct memory_data *next;
}memory_data;
typedef struct cache_line{
	int tag;
	int valid;
	int dirty;
	int block;
}line;
typedef struct cache{
	line **table;
	int *head;//head == oldest cache line
}cache;

void set_cache(cache* c){
	index_size = cache_size/block_size/set_size;
	c->table = (line**)malloc(index_size * sizeof(line*));
	c->head = (int*)malloc(index_size * sizeof(int));
	for(int i=0;i<index_size;i++){
		c->table[i]=(line*)malloc(set_size*sizeof(line));
		memset(c->table[i],0,sizeof(cache));
		memset(c->head[i],-1,sizeof(int));
	}

}

void c_write(cache* c, int ml, int data,memory_data* root){
	int tag = ml/index_size,
	    index = ml%index_size;
	int d_exist=0;
	memory_data *current = root;
	memory_data *new=NULL;
	//hit
	for(int i=0;i<set_size;i++){
		if(c->table[index][i].tag == tag && c->table[index][i].valid == 1){
			c->table[index][i].block = data;
			c->table[index][i].dirty = 1;
			return;
		}
	}
	//miss
	while(current -> next != NULL){ //exist in memory
		if(current->next->index == ml){
			//available space in cache
			for(int i=0;i<set_size;i++){
                		if(c->table[index][i].tag == 0 && c->table[index][i].valid == 0){
                        		c->table[index][i].tag = tag;
                        		c->table[index][i].valid = 1;
                        		c->table[index][i].dirty = 1;
                        		c->table[index][i].block = data;
                        		c->head[index]=0;
					return;
				}
			}
			//not available space in cache = kick out oldest(head) cache line
			
			// update data from cache to memory
			memory_data *kickout = root;
			while(kickout->index != c->table[index][c->head[index]].tag + index){
				kickout = kickout->next;
			}
			kickout->data = c->table[index][c->head[index]].block;

			//kick out
			c->table[index][c->head[index]].tag = tag;
			c->table[index][c->head[index]].valid = 1;
			c->table[index][c->head[index]].dirty = 1;
			c->table[index][c->head[index]].block = data;
			c->head[index] = (c->head[index]+1)%set_size;
			d_exist = 1;
			break;
		}
		current = current->next;
	}
	if(d_exist==0){ // not exist in memory 
		new = (memory_data*)malloc(sizeof(memory_data));
		new -> index = ml;
		new -> data = data;
		new -> next = NULL;
		current = root;
		while(current->next != NULL){
			current = current -> next;
		}
		current->next = new;
		
		//available space in cache
                for(int i=0;i<set_size;i++){
                        if(c->table[index][i].tag == 0 && c->table[index][i].valid == 0){
                                c->table[index][i].tag = tag;
                                c->table[index][i].valid = 1;
                                c->table[index][i].dirty = 1;
                                c->table[index][i].block = data;
                                c->head[index]=0;
				return;
                        }
                }
                //not available space in cache = kick ount oldest(head) cache line
		
		// update data from cache to memory
                memory_data *kickout = root;
                while(kickout->index != c->table[index][c->head[index]].tag + index){
                        kickout = kickout->next;
                }
                kickout->data = c->table[index][c->head[index]].block;

                //kick out
                c->table[index][c->head[index]].tag = tag;
                c->table[index][c->head[index]].valid = 1;
                c->table[index][c->head[index]].dirty = 1;
                c->table[index][c->head[index]].block = data;
                c->head[index] = (c->head[index]+1)%set_size;
	}


}
void c_read(cache* c,int ml,memory_data* root){
	int tag = ml/index_size,
            index = ml%index_size;
        int d_exist=0;
        memory_data *current = root;
        memory_data *new=NULL;
        //hit
        for(int i=0;i<set_size;i++){
                if(c->table[index][i].tag == tag && c->table[index][i].valid == 1){
                        return;
                }
        }
        //miss
        while(current -> next != NULL){ //exist in memory
                if(current->next->index == ml){
                        //available space in cache
                        for(int i=0;i<set_size;i++){
                                if(c->table[index][i].tag == 0 && c->table[index][i].valid == 0){
                                        c->table[index][i].tag = tag;
                                        c->table[index][i].valid = 1;
                                        c->table[index][i].dirty = 0;
                                        c->table[index][i].block = current->next->data;
                                        c->head[index]=0;
                                        return;
                                }
                        }
                        //not available space in cache = kick out oldest(head) cache line

                        // update data from cache to memory if dirty = 1
			if(c->table[index][c->head[index]].dirty == 1){
                        	memory_data *kickout = root;
                        	while(kickout->index != c->table[index][c->head[index]].tag + index){
                                	kickout = kickout->next;
                        	}
                        	kickout->data = c->table[index][c->head[index]].block;
			}

                        //kick out
                        c->table[index][c->head[index]].tag = tag;
                        c->table[index][c->head[index]].valid = 1;
                        c->table[index][c->head[index]].dirty = 0;
                        c->table[index][c->head[index]].block = current->next->data;
                        c->head[index] = (c->head[index]+1)%set_size;
                        d_exist = 1;
                        break;
                }
                current = current->next;
        }
	if(d_exist==0){ // not exist in memory
                new = (memory_data*)malloc(sizeof(memory_data));
                new -> index = ml;
                new -> data = 0;
                new -> next = NULL;
                current = root;
                while(current->next != NULL){
                        current = current -> next;
                }
                current->next = new;

                //available space in cache
                for(int i=0;i<set_size;i++){
                        if(c->table[index][i].tag == 0 && c->table[index][i].valid == 0){
                                c->table[index][i].tag = tag;
                                c->table[index][i].valid = 1;
                                c->table[index][i].dirty = 0;
                                c->table[index][i].block = 0;
                                c->head[index]=0;
                                return;
                        }
                }
                //not available space in cache = kick ount oldest(head) cache line
		
		// update data from cache to memory if dirty = 1
                if(c->table[index][c->head[index]].dirty == 1){
                        memory_data *kickout = root;
                        while(kickout->index != c->table[index][c->head[index]].tag + index){
                                kickout = kickout->next;
                        }
                        kickout->data = c->table[index][c->head[index]].block;
                }

                //kick out
                c->table[index][c->head[index]].tag = tag;
                c->table[index][c->head[index]].valid = 1;
                c->table[index][c->head[index]].dirty = 0;
                c->table[index][c->head[index]].block = 0;
                c->head[index] = (c->head[index]+1)%set_size;
        }



}
void c_print(cache* c){
	for(int i=0;i<index_size;i++){
		printf("%d: ",i);
		for(int j=0;j<set_size;j++){
			printf("%8X %8X v:%d d:%d\n",c->table[i][j].tag,c->table[i][j].block,c->table[i][j].valid,c->table[i][j].dirty);
		}

	}
}
int main(int argc,char* argv[]){
	cache C;
	memory_data M;
	FILE* fp;
	int memory_location,data;
	char R_W;

	sscanf(argv[1],"-a=%d",&set_size);
	sscanf(argv[2],"-s=%d",&cache_size);
	sscanf(argv[3],"-b=%d",&block_size);
	sscanf(argv[4],"-f=%s",trace_file);

	set_cache(&C);
	M.next = NULL;

	fp = fopen(trace_file,"r");
	if(fp==NULL)
		perror("File open error");
	else{
		while(!feof(fp)){
			fscanf(fp,"%X %c",memory_location,&R_W);
			if(R_W=='W'){
				fscanf(fp,"%d",&data);
				c_write(&C,memory_location,data,&M);

			}
			else{
				c_read(&C,memory_location,&M);
			}
		}
		fclose(fp);
	}
	c_print(&C);
	
}
