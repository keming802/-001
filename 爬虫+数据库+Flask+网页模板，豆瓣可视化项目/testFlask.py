from flask import Flask,render_template
app=Flask(__name__)
import datetime
# @app.route('/')
# def hello_world():
#     return '你好,欢迎关岭'

@app.route('/')
def index2():
    time=datetime.date.today()
    name=['小张','小王','小赵']
    task={"任务":"打扫卫生","时间":"3小时"}
    return render_template("index2.html",var=time,list=name,task=task)

if __name__=='__main__':
    app.run(debug=True)


