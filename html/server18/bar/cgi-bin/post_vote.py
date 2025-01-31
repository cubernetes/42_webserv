#!/usr/bin/env python3

import cgi
import json
import os

# Upvote content with ID 123
# curl -X POST -d "content_id=123&vote=up" http://webserv/cgi-bin/vote.py

# Using -F should work too
# curl -F "content_id=123" -F "vote=up" http://webserv/cgi-bin/vote.py

def main():
    print("Content-Type: application/json")
    print()
    
    form = cgi.FieldStorage()
    
    content_id = form.getvalue('content_id')
    vote = form.getvalue('vote')
    
    if not content_id or vote not in ['up', 'down']:
        print(json.dumps({
            'status': 'error',
            'message': 'Invalid input'
        }))
        return
        
    votes_file = f'votes_{content_id}.json'
    
    try:
        if os.path.exists(votes_file):
            with open(votes_file, 'r') as f:
                votes = json.load(f)
        else:
            votes = {'up': 0, 'down': 0}
        
        votes[vote] += 1
        
        with open(votes_file, 'w') as f:
            json.dump(votes, f)
        
        print(json.dumps({
            'status': 'success',
            'data': votes
        }))
        
    except Exception as e:
        print(json.dumps({
            'status': 'error',
            'message': str(e)
        }))

if __name__ == "__main__":
    main()
