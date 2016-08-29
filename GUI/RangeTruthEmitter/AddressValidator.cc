#include "QtGui/QLineEdit"

#include "AddressValidator.h"

using namespace SideCar::GUI::RangeTruthEmitter;

AddressValidator::AddressValidator(QLineEdit* widget)
    : QRegExpValidator(

	// The following should match valid IP addresses and host names. NOTE: we also accept an empty string.
	//
	QRegExp("^("
                // 250-255 | 200-249 | 0-199 '.' (three times)
                //
                "("
                "(((25[0-5])|(2[0-4]\\d)|([0-1]?\\d{1,2}))[.]){3}"
                // 250-255 | 200-249 | 0-199 (last octet)
                //
                "((25[0-5])|(2[0-4]\\d)|([0-1]?\\d{1,2}))"
                ")"
                "|"
                // Alphanumeric names with optional domain(s)
                //
                "(([A-Za-z][-A-Za-z0-9]*\\.?){1,})"
                ")?$"), widget),
      widget_(widget),
      normalColor_(widget->palette().color(QPalette::Text))
{
    widget_->setValidator(this);
}

AddressValidator::State
AddressValidator::validate(QString& input, int& pos) const
{
    State state = Super::validate(input, pos);

    // This is how we could let the user know that the entry is currently invalid. I don' think it looks that
    // great or is intuitive.
    //
#if 1
    QPalette p = widget_->palette();
    
    p.setColor(QPalette::Text,
               (state == Acceptable) ? normalColor_ : Qt::red);
    widget_->setPalette(p);
#endif
    return state;
}
